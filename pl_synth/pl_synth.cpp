// include chugin header
#include "chugin.h"

#define PL_SYNTH_IMPLEMENTATION
#include "pl_synth.h"
#include "jsmn.h"

#define BEGIN_CLASS(type, base) QUERY->begin_class(QUERY, type, base)
#define END_CLASS() QUERY->end_class(QUERY)
#define CTOR(func) QUERY->add_ctor(QUERY, func)
#define DTOR(func) QUERY->add_dtor(QUERY, func)
#define MVAR(type, name, is_const) QUERY->add_mvar(QUERY, type, name, is_const)
#define MFUN(func, ret, name) QUERY->add_mfun(QUERY, func, ret, name)
#define SFUN(func, ret, name) QUERY->add_sfun(QUERY, func, ret, name)
#define SVAR(type, name, val) QUERY->add_svar(QUERY, type, name, true, val);
#define ARG(type, name) QUERY->add_arg(QUERY, type, name)
#define DOC_FUNC(doc) QUERY->doc_func(QUERY, doc)
#define DOC_CLASS(doc) QUERY->doc_class(QUERY, doc)
#define DOC_VAR(doc) QUERY->doc_var(QUERY, doc)
#define ADD_EX(path) QUERY->add_ex(QUERY, path)

#define GET_NEXT_INT_ARRAY(ptr) (*((Chuck_ArrayInt**&)ptr)++)
#define GET_NEXT_FLOAT_ARRAY(ptr) (*((Chuck_ArrayFloat**&)ptr)++)
#define GET_NEXT_VEC2_ARRAY(ptr) (*((Chuck_ArrayVec2**&)ptr)++)
#define GET_NEXT_VEC3_ARRAY(ptr) (*((Chuck_ArrayVec3**&)ptr)++)
#define GET_NEXT_VEC4_ARRAY(ptr) (*((Chuck_ArrayVec4**&)ptr)++)
#define GET_NEXT_OBJECT_ARRAY(ptr) (*((Chuck_ArrayInt**&)ptr)++)


CK_DLL_CTOR( pl_synth_ctor );
CK_DLL_DTOR( pl_synth_dtor );

CK_DLL_MFUN( pl_synth_load );
CK_DLL_TICKF(pl_synth_tick_multichannel);

t_CKINT pl_synth_data_offset = 0;


static Chuck_VM *ck_vm = NULL;
static CK_DL_API ck_api = NULL;

CK_DLL_INFO( pl_synth )
{
    QUERY->setinfo( QUERY, CHUGIN_INFO_CHUGIN_VERSION, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_AUTHORS, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_DESCRIPTION, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_URL, "" );
    QUERY->setinfo( QUERY, CHUGIN_INFO_EMAIL, "" );
}


CK_DLL_QUERY( pl_synth )
{
    // initialize srate
    ck_vm = QUERY->ck_vm(QUERY);
    ck_api = (QUERY->ck_api(QUERY));
    PL_SYNTH_SAMPLERATE = ck_api->vm->srate(ck_vm);

    // initialize waveform LUTs
    void *synth_tab = malloc(PL_SYNTH_TAB_SIZE);
    pl_synth_init((float*)synth_tab);

    QUERY->setname( QUERY, "pl_synth" );

    BEGIN_CLASS("pl_synth", "UGen_Multi" );

    pl_synth_data_offset = MVAR("int", "@__data", false);

    CTOR(pl_synth_ctor);
    DTOR(pl_synth_dtor);


    MFUN(pl_synth_load, "void", "load");
    ARG("string", "pl_synth_json");

    QUERY->add_ugen_funcf(QUERY, pl_synth_tick_multichannel, NULL,
                            0, // 0 channels in
                            2  // stereo out
    );

    END_CLASS();
    return TRUE;
}

static int parseNumber(jsmntok_t* token, const char* json_str) {
    char prim = json_str[token->start];
    if (prim == '-' || (prim >= '0' && prim <= '9')) { // number
        // assume int, not float
        return atoi(json_str+ token->start);
    } else {
        printf("pl_synth: unrecognized json number %.*s. Default to 0\n", token->end - token->start, json_str + token->start);
        return 0;
    }
}

static void printTok(jsmntok_t tok) {
    printf("type %d, start %d, end %d, size %d\n", tok.type, tok.start, tok.end, tok.size);
}

// TODO put in pl_synth.h
static pl_synth_rt_song_t* parseJson(const char* json_str) {
    pl_synth_rt_song_t* song = NULL;

    // determine number of tokens needed
    jsmn_parser parser;
    jsmn_init(&parser);
    int num_tokens = jsmn_parse(&parser, json_str, strlen(json_str), NULL, 0);

    // error handling
    if (num_tokens < 0) {
        switch (num_tokens) {
            case JSMN_ERROR_NOMEM:
                ck_api->vm->em_log(1, "pl_synth: Out of memory, json too large");
                break;
            case JSMN_ERROR_INVAL:
            case JSMN_ERROR_PART:
                ck_api->vm->em_log(1, "pl_synth: Invalid JSON");
                break;
        }
        return song;
    }

    jsmntok_t *tokens = (jsmntok_t *)malloc(sizeof(jsmntok_t) * num_tokens);

    jsmn_init(&parser);
    int parse_result = jsmn_parse(&parser, json_str, strlen(json_str), tokens, num_tokens);
    if (parse_result != num_tokens) {
        ck_api->vm->em_log(1, "pl_synth: Error parsing JSON");
    } else {
        // print all tokens
        // for (int json_elem_idx = 0; json_elem_idx < num_tokens; json_elem_idx++)
        // {
        //     jsmntok_t tok = tokens[json_elem_idx];
        //     // print tok
        //     printf("type %d, start %d, end %d, size %d\n", tok.type, tok.start, tok.end, tok.size);
        // }

        song = (pl_synth_rt_song_t*) calloc(1, sizeof(*song));

        jsmntok_t *song_token = tokens;
        assert(song_token->type == JSMN_ARRAY && song_token->size == 2);

        jsmntok_t *row_len_token = &tokens[1];
        assert(row_len_token->type == JSMN_PRIMITIVE);
        song->row_len = parseNumber(row_len_token, json_str);

        jsmntok_t *tracks_token = &tokens[2];
        assert(tracks_token->type == JSMN_ARRAY);
        song->num_tracks = tracks_token->size;
        song->tracks = (pl_synth_rt_track_t*)calloc(song->num_tracks, sizeof(*song->tracks));

        // int expected_size = token->size;
        printf("parsing song with %d tracks\n", song->num_tracks);
        int curr_tok_idx = 2;
        for (int track_idx = 0; track_idx < song->num_tracks; track_idx++)
        {
            pl_synth_rt_track_t* rt_track = &song->tracks[track_idx];
            pl_synth_track_t* track = &rt_track->track;

			int delay_samps = 32 * song->row_len;
			rt_track->delay_line_MALLOC = (int16_t*)calloc(delay_samps * 2, sizeof(*rt_track->delay_line_MALLOC));
			rt_track->delay_line_size = delay_samps;

            jsmntok_t track_tok = tokens[++curr_tok_idx];
            assert(track_tok.size == 3); // 3 fields
            jsmntok_t instr_tok = tokens[++curr_tok_idx];
            { // parse into instr
                pl_synth_t* s= &track->synth;
                // all other params beyond instr_tok.size are assumed to be zero
                for (int i = 0; i < instr_tok.size; ++i) {
                    int val = parseNumber(&tokens[++curr_tok_idx], json_str);
                    switch (i) {
                        case 0: s->osc0_oct = val; break;
                        case 1: s->osc0_det = val; break;
                        case 2: s->osc0_detune = val; break;
                        case 3: s->osc0_xenv = val; break;
                        case 4: s->osc0_vol = val; break;
                        case 5: s->osc0_waveform = val; break;
                        case 6: s->osc1_oct = val; break;
                        case 7: s->osc1_det = val; break;
                        case 8: s->osc1_detune = val; break;
                        case 9: s->osc1_xenv = val; break;
                        case 10: s->osc1_vol = val; break;
                        case 11: s->osc1_waveform = val; break;
                        case 12: s->noise_fader = val; break;
                        case 13: s->env_attack = val; break;
                        case 14: s->env_sustain = val; break;
                        case 15: s->env_release = val; break;
                        case 16: s->env_master = val; break;
                        case 17: {
                            s->fx_filter = val; 
                            // printf("parsed filter %d\n", val);
                        }break;
                        case 18: s->fx_freq = val; break;
                        case 19: {
                            s->fx_resonance = val; 
                            // printf("filter res %d\n", val);
                        }break;
                        case 20: s->fx_delay_time = val; break;
                        case 21: s->fx_delay_amt = val; break;
                        case 22: s->fx_pan_freq = val; break;
                        case 23: s->fx_pan_amt = val; break;
                        case 24: s->lfo_osc_freq = val; break;
                        case 25: s->lfo_fx_freq = val; break;
                        case 26: s->lfo_freq = val; break;
                        case 27: s->lfo_amt = val; break;
                        case 28: s->lfo_waveform = val; break;
                        default: assert(0);
                    }
                }
            }

            jsmntok_t seq_tok = tokens[++curr_tok_idx];
            track->sequence_len = seq_tok.size;
            track->sequence = (uint8_t*)calloc(seq_tok.size, sizeof(*track->sequence));
            for (int seq_idx = 0; seq_idx < seq_tok.size; ++seq_idx) {
                track->sequence[seq_idx] = parseNumber(&tokens[++curr_tok_idx], json_str);
            }

            
            jsmntok_t pattern_list_tok = tokens[++curr_tok_idx];
            printf("track: %d has %d patterns\n", track_idx, pattern_list_tok.size);
            track->patterns = (pl_synth_pattern_t*)calloc(pattern_list_tok.size, sizeof(*track->patterns));
            for (int pattern_idx = 0; pattern_idx < pattern_list_tok.size; ++pattern_idx) {
                jsmntok_t pattern_tok = tokens[++curr_tok_idx];
                pl_synth_pattern_t* pattern = track->patterns + pattern_idx;
                for (int note_idx = 0; note_idx < pattern_tok.size; ++note_idx) {
                    pattern->notes[note_idx] = parseNumber(&tokens[++curr_tok_idx], json_str);
                }
            }
        }
        assert(curr_tok_idx == num_tokens - 1);
    }

    // sanity check print everything
    // printf("row_len: %d\n", song->row_len);
    // printf("samps: %d\n", song->samps);
    // printf("num_tracks: %d\n", song->num_tracks);
    // for (int i = 0; i < num_tracks; i++) {
    //     pl_synth_rt_track_t* rt_track = song->tracks + i;
    // }

    free(tokens);
    return song;
}


// implementation for the default constructor
CK_DLL_CTOR( pl_synth_ctor )
{
    // OBJ_MEMBER_INT( SELF, pl_synth_data_offset ) = 0;
    // pl_synth * __obj = new pl_synth( API->vm->srate(VM) );
    // OBJ_MEMBER_INT( SELF, pl_synth_data_offset ) = (t_CKINT)__obj;
}


// implementation for the destructor
CK_DLL_DTOR( pl_synth_dtor )
{
    // pl_synth * __obj = (pl_synth *)OBJ_MEMBER_INT( SELF, pl_synth_data_offset );
    // CK_SAFE_DELETE( __obj );
    // OBJ_MEMBER_INT( SELF, pl_synth_data_offset ) = 0;
}

CK_DLL_MFUN( pl_synth_load )
{
    // TODO free old data
    Chuck_String* ck_json_str = GET_NEXT_STRING(ARGS);
    const char* json_str = API->object->str(ck_json_str);
    OBJ_MEMBER_INT( SELF, pl_synth_data_offset ) = (t_CKINT) parseJson(json_str);
}


// implementation for tick function (relevant only for UGens)
CK_DLL_TICKF( pl_synth_tick_multichannel ) {
    pl_synth_rt_song_t* song = (pl_synth_rt_song_t*)OBJ_MEMBER_INT( SELF, pl_synth_data_offset );
    int16_t output[2] = {0};
    if (song) {
        ++song->samps;
        for (int track_idx = 0; track_idx < song->num_tracks; track_idx++)
            pl_synth_track_tick(song->tracks + track_idx, output, song->row_len, song->samps);

        // not clamping since chuck should already do that
        // output[0] = pl_synth_clamp_s16(output[0]);
        // output[1] = pl_synth_clamp_s16(output[1]);

        out[0] = output[0] / 32768.0f;
        out[1] = output[1] / 32768.0f;
        // printf("%d ", output[0]);
    }

    return TRUE;
}
