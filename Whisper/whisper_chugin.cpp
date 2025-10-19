// include chugin header
#include "chugin.h"
#include "whisper.h"
#include "stdio.h"
#include "tinycthread.h"
#include "tinycthread.c"

#include <chrono> // @TODO remove

#define ASSERT(expression)                                                             \
    {                                                                                  \
        if (!(expression)) {                                                           \
            printf("Assertion(%s) failed: file \"%s\", line %d\n", #expression,        \
                   __FILE__, __LINE__);                                                \
        }                                                                              \
    }
#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))
#define ZERO_ARRAY(array) (memset((array), 0, sizeof(array)))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN(hi, MAX(lo, x)))

static Chuck_VM* ck_vm = NULL;
static CK_DL_API ck_api = NULL;
static t_CKUINT ck_srate = 0;
static CBufferSimple* ck_event_queue = NULL;

struct WhisperChugin {
    char* model_path_MALLOC;

    float input[(int) (WHISPER_SAMPLE_RATE * 5)]; 
    float input_size;
    int input_pos;  // write idx

    // where the samples are accumulated for inference
    // currently fixed to a window size of 5 seconds 
    // TODO make this a parameter, with the max being Whisper's internal max of 30sec
    float accum[(int) (WHISPER_SAMPLE_RATE * 5)]; 
    int accum_size; // how filled accum currently is (in samps)
    bool doing_inference; // set by inference thread
    char* last_inference_text_MALLOC; // set by inference thread
    int last_inference_text_size;

    // synchronization
    mtx_t accum_mutex;   
    bool transcribe_signal; // predicate for conditoin variable
    cnd_t transcribe_cond;
    mtx_t transcribe_mutex;

    SAMPLE prev_tick_in; // for interpolation when downsampling chuck input
    SAMPLE last_downsample;  // last value written into input buffer
    float ticks_till_next_samp = 1.0f; // used for downsampling bookeeping

    Chuck_Event* transcribe_event; // broadcast when transcription finishes

    // for now we have fixed step=500ms, len=5000ms, keep=200ms
    float keep_sec = .2f;
};

CK_DLL_CTOR( whisper_ctor );
CK_DLL_DTOR( whisper_dtor );
CK_DLL_MFUN( whisper_get_text );

CK_DLL_MFUN( whisper_transcribe );

CK_DLL_TICK( whisper_tick );
t_CKINT whisper_data_offset = 0;


CK_DLL_INFO( Whisper )
{
    QUERY->setinfo( QUERY, CHUGIN_INFO_CHUGIN_VERSION, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_AUTHORS, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_DESCRIPTION, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_URL, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_EMAIL, "" );
}

CK_DLL_QUERY( Whisper )
{
    // nocheckin
    whisper_context_params cparams = whisper_context_default_params(); 
    printf("use_gpu: %d\n", cparams.use_gpu);

    ck_vm = QUERY->ck_vm(QUERY);
    ck_api = (QUERY->ck_api(QUERY));
    ck_srate = ck_api->vm->srate(ck_vm);
    ck_event_queue = ck_api->vm->create_event_buffer(ck_vm);

    QUERY->setname( QUERY, "Whisper" );

    QUERY->begin_class( QUERY, "Whisper", "UGen" );

    static t_CKINT whisper_srate = WHISPER_SAMPLE_RATE;
    QUERY->add_svar( QUERY, "int", "SRATE", true, &whisper_srate );

    QUERY->add_ctor( QUERY, whisper_ctor );
    QUERY->add_arg(QUERY, "string", "path_to_model");

    QUERY->add_dtor( QUERY, whisper_dtor );

    // for UGens only: add tick function
    // NOTE a non-UGen class should remove or comment out this next line
    QUERY->add_ugen_func( QUERY, whisper_tick, NULL, 1, 1 );
    // NOTE: if this is to be a UGen with more than 1 channel,
    // e.g., a multichannel UGen -- will need to use add_ugen_funcf()
    // and declare a tickf function using CK_DLL_TICKF

    QUERY->add_mfun(QUERY, whisper_transcribe, "Event", "transcribe");

    QUERY->add_mfun( QUERY, whisper_get_text, "string", "text" );
    
    // this reserves a variable in the ChucK internal class to store 
    // referene to the c++ class we defined above
    whisper_data_offset = QUERY->add_mvar( QUERY, "int", "@w_data", false );

    // ------------------------------------------------------------------------
    // end the class definition
    // IMPORTANT: this MUST be called to each class definition!
    // ------------------------------------------------------------------------
    QUERY->end_class( QUERY );

    // wasn't that a breeze?
    return TRUE;
}

static int WhisperInferenceThread(void* arg)
{
    WhisperChugin* w = (WhisperChugin*) arg;
    // whisper init
    const char* model = w->model_path_MALLOC;
    struct whisper_context_params cparams = whisper_context_default_params();
    struct whisper_context * ctx = whisper_init_from_file_with_params(model, cparams);
    if (ctx == nullptr) {
        printf("error: failed to initialize whisper context with model %s\n", model);
        return thrd_error;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress   = false;
    wparams.print_special    = false;
    wparams.print_realtime   = false;
    wparams.print_timestamps = false;
    wparams.single_segment   = true;
    wparams.n_threads        = 4; // @TODO maybe use std::thread::hardware_concurrency());

    bool loop = true;
    while (loop) {
        mtx_lock(&w->transcribe_mutex);
        while (!w->transcribe_signal) {
            // @TODO destroying the cnd_variable is probably a terrible way to signal cleanup
            if (cnd_wait(&w->transcribe_cond, &w->transcribe_mutex) != thrd_success) {
                loop = false;
                break;
            }
        }
        w->transcribe_signal = false;
        mtx_unlock(&w->transcribe_mutex);
        if (!loop) break;


        // @TODO detect when to exit, clean up resources, broadcast Chuck event

        mtx_lock(&w->accum_mutex);
        w->doing_inference = true;
        mtx_unlock(&w->accum_mutex);

        auto whisper_bef  = std::chrono::high_resolution_clock::now();
        if (whisper_full(ctx, wparams, w->accum, w->accum_size) != 0) {
            printf("Whisper failed to process audio\n");
            return thrd_error;
        }
        auto whisper_aft  = std::chrono::high_resolution_clock::now();
        auto whisper_time = std::chrono::duration_cast<std::chrono::milliseconds>(whisper_bef - whisper_aft).count();
        // printf(">>whisper_full(...) took %lldms\n", whisper_time);

        const int n_segments = whisper_full_n_segments(ctx);
        ASSERT(n_segments <= 1);
        const char * text = NULL;
        int text_len = 0;
        if (n_segments > 0) {
            text = whisper_full_get_segment_text(ctx, 0);
            text_len = strlen(text);

            // printf("%s\n", text);
            // idk if chuck string creation is threadsafe, so going to strdup this into 
            // the whisperChugin struct where it can be read into chuck on the audio vm thread
        }
        mtx_lock(&w->accum_mutex);
        w->doing_inference = false;
        if (text) {
           if (w->last_inference_text_size < text_len + 1) {
                int new_size = MAX(text_len + 1, w->last_inference_text_size * 2);
                w->last_inference_text_MALLOC = (char* )realloc(w->last_inference_text_MALLOC, new_size);
                w->last_inference_text_size = new_size;
           }
           snprintf(w->last_inference_text_MALLOC, w->last_inference_text_size, "%s", text);
        }
        mtx_unlock(&w->accum_mutex);

        // broadcast that inference is done
        ck_api->vm->queue_event(ck_vm, w->transcribe_event, 1, ck_event_queue);
    }

    {
        // free stuff
        printf("whisper inference thread cleanup\n");
        whisper_print_timings(ctx);
        whisper_free(ctx);
        mtx_destroy(&w->accum_mutex);
        mtx_destroy(&w->transcribe_mutex);
        free(w->model_path_MALLOC);
        free(w->last_inference_text_MALLOC);
        free(w);
        // @LEAK is release() threadsafe? for now we're just gonna leak it...
        // API->object->release( (Chuck_Object*) w_obj->transcribe_event);
    }
    return thrd_success;
}


// implementation for the default constructor
CK_DLL_CTOR( whisper_ctor ) {
    Chuck_String* model_ckstr = GET_NEXT_STRING(ARGS);

    WhisperChugin* w = (WhisperChugin*)malloc(sizeof(WhisperChugin));
    *w = {}; // init
    w->model_path_MALLOC = strdup(API->object->str(model_ckstr));

    Chuck_DL_Api::Type cktype = API->type->lookup(VM, "Event");
    w->transcribe_event = (Chuck_Event*) API->object->create(SHRED, cktype, true);

    OBJ_MEMBER_INT( SELF, whisper_data_offset ) = (t_CKINT) w;

    // init thread stuff
    mtx_init(&w->accum_mutex, mtx_plain);
    mtx_init(&w->transcribe_mutex, mtx_plain);
    cnd_init(&w->transcribe_cond);

    thrd_t t;
    thrd_create(&t, WhisperInferenceThread, w);
    thrd_detach(t); // can't block on a join, so detach now 
}



CK_DLL_DTOR( whisper_dtor ) { 
    WhisperChugin * w = (WhisperChugin *)OBJ_MEMBER_INT(SELF, whisper_data_offset);
    OBJ_MEMBER_INT(SELF, whisper_data_offset) = 0;

    mtx_lock(&w->transcribe_mutex);
    w->transcribe_signal = true;
    mtx_unlock(&w->transcribe_mutex);
    cnd_broadcast(&w->transcribe_cond); // wake up all associated threads for cleanup

    cnd_destroy(&w->transcribe_cond); // if the thread wasn't waiting, destroy it so it breaks out the next time it tries to wait
}


// implementation for tick function (relevant only for UGens)
// #define CK_DLL_TICK(name) CK_DLL_EXPORT(t_CKBOOL) name( Chuck_Object * SELF, SAMPLE in, SAMPLE * out, CK_DL_API API )
CK_DLL_TICK( whisper_tick )
{
    WhisperChugin * w_obj = (WhisperChugin *)OBJ_MEMBER_INT(SELF, whisper_data_offset);
    w_obj->ticks_till_next_samp -= 1;

    // downsample and interpolate into input buffer
    if (w_obj->ticks_till_next_samp <= 0) {
        ASSERT(w_obj->ticks_till_next_samp > -1);

        // linear interpolation
        w_obj->last_downsample = w_obj->prev_tick_in + (in - w_obj->prev_tick_in) * (1 + w_obj->ticks_till_next_samp);
        w_obj->input[w_obj->input_pos++] = w_obj->last_downsample;

        // reset counter
        w_obj->ticks_till_next_samp = (float) ck_srate / WHISPER_SAMPLE_RATE;

        // wrap write pos
        int input_cap = ARRAY_LENGTH(w_obj->input);
        if (w_obj->input_pos >= input_cap) {
            w_obj->input_pos = 0;
        }
        // update total len
        w_obj->input_size++;
        if (w_obj->input_pos > input_cap) w_obj->input_size = input_cap;
    }

    w_obj->prev_tick_in = in;

    // TODO should this be a UAna?
    // hear the downsampled output
    *out = w_obj->last_downsample;
    return true;
}

CK_DLL_MFUN( whisper_transcribe )
{
    WhisperChugin * w = (WhisperChugin *)OBJ_MEMBER_INT(SELF, whisper_data_offset);
    RETURN->v_object = (Chuck_Object*) w->transcribe_event;

    mtx_lock(&w->accum_mutex);
    // check if transcribe(...) is already outstanding, if so error out.
    if (w->doing_inference) {
        printf("Whisper error: calling transcribe before previous inference is finished. Returning early.\n");
        mtx_unlock(&w->accum_mutex);
        return;
    }

    // copy the new input into accum
    int accum_cap = ARRAY_LENGTH(w->accum);
    int input_cap = ARRAY_LENGTH(w->input);
    int n_samples_keep = w->keep_sec * WHISPER_SAMPLE_RATE;
    ASSERT(input_cap <= accum_cap);
    ASSERT(w->input_size <= input_cap);
    ASSERT(n_samples_keep <= accum_cap);
    bool would_overflow = w->input_size > (accum_cap - w->accum_size);

    // reset the accum, keeping the last bit so we don't lose 
    // context in the middle of a word
    int overlap_window_samps = CLAMP(n_samples_keep, 0, accum_cap - w->input_size);
    if (would_overflow) {
        ASSERT(w->accum_size > overlap_window_samps);
        // overlap window
        memmove(w->accum, w->accum + w->accum_size - overlap_window_samps, overlap_window_samps * sizeof(float));
        // zero the rest
        memset(w->accum + overlap_window_samps, 0, (accum_cap - overlap_window_samps) * sizeof(float));
        w->accum_size = overlap_window_samps;
    }

    // copy over the contents from input buffer
    ASSERT(accum_cap - w->accum_size >= w->input_size);
    int input_read_pos = w->input_pos - w->input_size;
    if (input_read_pos < 0) {
        input_read_pos += input_cap;
        // need to wrap
        int size1 = input_cap - input_read_pos;
        int size2 = w->input_size - size1; ASSERT(size2 > 0);
        memcpy(w->accum + w->accum_size, w->input + input_read_pos, sizeof(float) * size1 );
        w->accum_size += size1;
        memcpy(w->accum + w->accum_size, w->input, sizeof(float) * size2);
        w->accum_size += size2;
    } else {
        memcpy(w->accum + w->accum_size, w->input + input_read_pos, sizeof(float) * w->input_size);
        w->accum_size += w->input_size;
    }
    w->input_size = 0; // just flushed the input buffer, reset size
    ASSERT(w->accum_size <= accum_cap);

    mtx_unlock(&w->accum_mutex);

    // wake up inference thread
    mtx_lock(&w->transcribe_mutex);
    w->transcribe_signal = true;
    mtx_unlock(&w->transcribe_mutex);
    cnd_broadcast(&w->transcribe_cond);
}


// example implementation for getter
CK_DLL_MFUN(whisper_get_text)
{
    WhisperChugin * w = (WhisperChugin *)OBJ_MEMBER_INT(SELF, whisper_data_offset);

    mtx_lock(&w->accum_mutex);
    RETURN->v_object = (Chuck_Object*) API->object->create_string(VM, w->last_inference_text_MALLOC ? w->last_inference_text_MALLOC : "", false);
    mtx_unlock(&w->accum_mutex);
}
