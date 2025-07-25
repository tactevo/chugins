// clang-format off
#include "chugin.h"
#include <stdio.h>

// #define DOOM_IMPLEMENT_PRINT
// #define DOOM_IMPLEMENT_MALLOC
// #define DOOM_IMPLEMENT_FILE_IO
// #define DOOM_IMPLEMENT_SOCKETS
// #define DOOM_IMPLEMENT_GETTIME
// #define DOOM_IMPLEMENT_EXIT
// #define DOOM_IMPLEMENT_GETENV
// #define DOOM_IMPLEMENTATION
#include "PureDOOM.h"

/* Printable keys */
#define GLFW_KEY_SPACE              32
#define GLFW_KEY_APOSTROPHE         39  /* ' */
#define GLFW_KEY_COMMA              44  /* , */
#define GLFW_KEY_MINUS              45  /* - */
#define GLFW_KEY_PERIOD             46  /* . */
#define GLFW_KEY_SLASH              47  /* / */
#define GLFW_KEY_0                  48
#define GLFW_KEY_1                  49
#define GLFW_KEY_2                  50
#define GLFW_KEY_3                  51
#define GLFW_KEY_4                  52
#define GLFW_KEY_5                  53
#define GLFW_KEY_6                  54
#define GLFW_KEY_7                  55
#define GLFW_KEY_8                  56
#define GLFW_KEY_9                  57
#define GLFW_KEY_SEMICOLON          59  /* ; */
#define GLFW_KEY_EQUAL              61  /* = */
#define GLFW_KEY_A                  65
#define GLFW_KEY_B                  66
#define GLFW_KEY_C                  67
#define GLFW_KEY_D                  68
#define GLFW_KEY_E                  69
#define GLFW_KEY_F                  70
#define GLFW_KEY_G                  71
#define GLFW_KEY_H                  72
#define GLFW_KEY_I                  73
#define GLFW_KEY_J                  74
#define GLFW_KEY_K                  75
#define GLFW_KEY_L                  76
#define GLFW_KEY_M                  77
#define GLFW_KEY_N                  78
#define GLFW_KEY_O                  79
#define GLFW_KEY_P                  80
#define GLFW_KEY_Q                  81
#define GLFW_KEY_R                  82
#define GLFW_KEY_S                  83
#define GLFW_KEY_T                  84
#define GLFW_KEY_U                  85
#define GLFW_KEY_V                  86
#define GLFW_KEY_W                  87
#define GLFW_KEY_X                  88
#define GLFW_KEY_Y                  89
#define GLFW_KEY_Z                  90
#define GLFW_KEY_LEFT_BRACKET       91  /* [ */
#define GLFW_KEY_BACKSLASH          92  /* \ */
#define GLFW_KEY_RIGHT_BRACKET      93  /* ] */
#define GLFW_KEY_GRAVE_ACCENT       96  /* ` */
#define GLFW_KEY_WORLD_1            161 /* non-US #1 */
#define GLFW_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define GLFW_KEY_ESCAPE             256
#define GLFW_KEY_ENTER              257
#define GLFW_KEY_TAB                258
#define GLFW_KEY_BACKSPACE          259
#define GLFW_KEY_INSERT             260
#define GLFW_KEY_DELETE             261
#define GLFW_KEY_RIGHT              262
#define GLFW_KEY_LEFT               263
#define GLFW_KEY_DOWN               264
#define GLFW_KEY_UP                 265
#define GLFW_KEY_PAGE_UP            266
#define GLFW_KEY_PAGE_DOWN          267
#define GLFW_KEY_HOME               268
#define GLFW_KEY_END                269
#define GLFW_KEY_CAPS_LOCK          280
#define GLFW_KEY_SCROLL_LOCK        281
#define GLFW_KEY_NUM_LOCK           282
#define GLFW_KEY_PRINT_SCREEN       283
#define GLFW_KEY_PAUSE              284
#define GLFW_KEY_F1                 290
#define GLFW_KEY_F2                 291
#define GLFW_KEY_F3                 292
#define GLFW_KEY_F4                 293
#define GLFW_KEY_F5                 294
#define GLFW_KEY_F6                 295
#define GLFW_KEY_F7                 296
#define GLFW_KEY_F8                 297
#define GLFW_KEY_F9                 298
#define GLFW_KEY_F10                299
#define GLFW_KEY_F11                300
#define GLFW_KEY_F12                301
#define GLFW_KEY_F13                302
#define GLFW_KEY_F14                303
#define GLFW_KEY_F15                304
#define GLFW_KEY_F16                305
#define GLFW_KEY_F17                306
#define GLFW_KEY_F18                307
#define GLFW_KEY_F19                308
#define GLFW_KEY_F20                309
#define GLFW_KEY_F21                310
#define GLFW_KEY_F22                311
#define GLFW_KEY_F23                312
#define GLFW_KEY_F24                313
#define GLFW_KEY_F25                314
#define GLFW_KEY_KP_0               320
#define GLFW_KEY_KP_1               321
#define GLFW_KEY_KP_2               322
#define GLFW_KEY_KP_3               323
#define GLFW_KEY_KP_4               324
#define GLFW_KEY_KP_5               325
#define GLFW_KEY_KP_6               326
#define GLFW_KEY_KP_7               327
#define GLFW_KEY_KP_8               328
#define GLFW_KEY_KP_9               329
#define GLFW_KEY_KP_DECIMAL         330
#define GLFW_KEY_KP_DIVIDE          331
#define GLFW_KEY_KP_MULTIPLY        332
#define GLFW_KEY_KP_SUBTRACT        333
#define GLFW_KEY_KP_ADD             334
#define GLFW_KEY_KP_ENTER           335
#define GLFW_KEY_KP_EQUAL           336
#define GLFW_KEY_LEFT_SHIFT         340
#define GLFW_KEY_LEFT_CONTROL       341
#define GLFW_KEY_LEFT_ALT           342
#define GLFW_KEY_LEFT_SUPER         343
#define GLFW_KEY_RIGHT_SHIFT        344
#define GLFW_KEY_RIGHT_CONTROL      345
#define GLFW_KEY_RIGHT_ALT          346
#define GLFW_KEY_RIGHT_SUPER        347
#define GLFW_KEY_MENU               348

void hexDump(const char* desc, const void* addr, const int len)
{
    const static int perLine = 16;

    int i;
    unsigned char buff[perLine + 1];
    const unsigned char* pc = (const unsigned char*)addr;

    // Output description if given.
    if (desc != NULL) printf("%s:\n", desc);

    // Length checks.
    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %d\n", len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of perLine means new or first line (with line offset).
        if ((i % perLine) == 0) {
            // Only print previous-line ASCII buffer for lines beyond first.
            if (i != 0) printf("  %s\n", buff);
            // Output the offset of current line.
            printf("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And buffer a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
            buff[i % perLine] = '.';
        else
            buff[i % perLine] = pc[i];
        buff[(i % perLine) + 1] = '\0';
    }

    // Pad out last line if not exactly perLine characters.

    while ((i % perLine) != 0) {
        printf("   ");
        i++;
    }

    // And print the final ASCII buffer.
    printf("  %s\n", buff);
}


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

CK_DL_API g_chuglAPI;
Chuck_VM *g_chuglVM;
Chuck_DL_Api::Type ckint_array_type;
// Chuck_DL_Api::Type float_array;
// Chuck_DL_Api::Type vec2_array;
// Chuck_DL_Api::Type vec3_array;
// Chuck_DL_Api::Type vec4_array;

Chuck_Object *chugin_createCkObj(const char *type_name, bool add_ref,
                                 Chuck_VM_Shred *shred = NULL)
{
    Chuck_DL_Api::Type cktype = g_chuglAPI->type->lookup(g_chuglVM, type_name);
    if (shred)
    {
        return g_chuglAPI->object->create(shred, cktype, add_ref);
    }
    else
    {
        return g_chuglAPI->object->create_without_shred(g_chuglVM, cktype, add_ref);
    }
}

doom_key_t chugl_keycode_to_doom_key(t_CKINT code)
{
    switch (code)
    {
        case GLFW_KEY_TAB: return DOOM_KEY_TAB;
        case GLFW_KEY_ENTER: return DOOM_KEY_ENTER;
        case GLFW_KEY_ESCAPE: return DOOM_KEY_ESCAPE;
        case GLFW_KEY_SPACE: return DOOM_KEY_SPACE;
        case GLFW_KEY_APOSTROPHE: return DOOM_KEY_APOSTROPHE;
        case GLFW_KEY_KP_MULTIPLY: return DOOM_KEY_MULTIPLY;
        case GLFW_KEY_COMMA: return DOOM_KEY_COMMA;
        case GLFW_KEY_MINUS: return DOOM_KEY_MINUS;
        case GLFW_KEY_PERIOD: return DOOM_KEY_PERIOD;
        case GLFW_KEY_SLASH: return DOOM_KEY_SLASH;
        case GLFW_KEY_0: return DOOM_KEY_0;
        case GLFW_KEY_1: return DOOM_KEY_1; 
        case GLFW_KEY_2: return DOOM_KEY_2;
        case GLFW_KEY_3: return DOOM_KEY_3;
        case GLFW_KEY_4: return DOOM_KEY_4;
        case GLFW_KEY_5: return DOOM_KEY_5;
        case GLFW_KEY_6: return DOOM_KEY_6;
        case GLFW_KEY_7: return DOOM_KEY_7;
        case GLFW_KEY_8: return DOOM_KEY_8;
        case GLFW_KEY_9: return DOOM_KEY_9;
        case GLFW_KEY_SEMICOLON: return DOOM_KEY_SEMICOLON;
        case GLFW_KEY_EQUAL: return DOOM_KEY_EQUALS;
        case GLFW_KEY_LEFT_BRACKET: return DOOM_KEY_LEFT_BRACKET;
        case GLFW_KEY_RIGHT_BRACKET: return DOOM_KEY_RIGHT_BRACKET;
        case GLFW_KEY_A: return DOOM_KEY_A;
        case GLFW_KEY_B: return DOOM_KEY_B;
        case GLFW_KEY_C: return DOOM_KEY_C;
        case GLFW_KEY_D: return DOOM_KEY_D;
        case GLFW_KEY_E: return DOOM_KEY_E;
        case GLFW_KEY_F: return DOOM_KEY_F;
        case GLFW_KEY_G: return DOOM_KEY_G;
        case GLFW_KEY_H: return DOOM_KEY_H;
        case GLFW_KEY_I: return DOOM_KEY_I;
        case GLFW_KEY_J: return DOOM_KEY_J;
        case GLFW_KEY_K: return DOOM_KEY_K;
        case GLFW_KEY_L: return DOOM_KEY_L;
        case GLFW_KEY_M: return DOOM_KEY_M;
        case GLFW_KEY_N: return DOOM_KEY_N;
        case GLFW_KEY_O: return DOOM_KEY_O;
        case GLFW_KEY_P: return DOOM_KEY_P;
        case GLFW_KEY_Q: return DOOM_KEY_Q;
        case GLFW_KEY_R: return DOOM_KEY_R;
        case GLFW_KEY_S: return DOOM_KEY_S;
        case GLFW_KEY_T: return DOOM_KEY_T;
        case GLFW_KEY_U: return DOOM_KEY_U;
        case GLFW_KEY_V: return DOOM_KEY_V;
        case GLFW_KEY_W: return DOOM_KEY_W;
        case GLFW_KEY_X: return DOOM_KEY_X;
        case GLFW_KEY_Y: return DOOM_KEY_Y;
        case GLFW_KEY_Z: return DOOM_KEY_Z;
        case GLFW_KEY_BACKSPACE: return DOOM_KEY_BACKSPACE;
        case GLFW_KEY_LEFT_CONTROL:
        case GLFW_KEY_RIGHT_CONTROL: return DOOM_KEY_CTRL;
        case GLFW_KEY_LEFT: return DOOM_KEY_LEFT_ARROW;
        case GLFW_KEY_UP: return DOOM_KEY_UP_ARROW;
        case GLFW_KEY_RIGHT: return DOOM_KEY_RIGHT_ARROW;
        case GLFW_KEY_DOWN: return DOOM_KEY_DOWN_ARROW;
        case GLFW_KEY_LEFT_SHIFT:
        case GLFW_KEY_RIGHT_SHIFT: return DOOM_KEY_SHIFT;
        case GLFW_KEY_LEFT_ALT:
        case GLFW_KEY_RIGHT_ALT: return DOOM_KEY_ALT;
        case GLFW_KEY_F1: return DOOM_KEY_F1;
        case GLFW_KEY_F2: return DOOM_KEY_F2;
        case GLFW_KEY_F3: return DOOM_KEY_F3;
        case GLFW_KEY_F4: return DOOM_KEY_F4;
        case GLFW_KEY_F5: return DOOM_KEY_F5;
        case GLFW_KEY_F6: return DOOM_KEY_F6;
        case GLFW_KEY_F7: return DOOM_KEY_F7;
        case GLFW_KEY_F8: return DOOM_KEY_F8;
        case GLFW_KEY_F9: return DOOM_KEY_F9;
        case GLFW_KEY_F10: return DOOM_KEY_F10;
        case GLFW_KEY_F11: return DOOM_KEY_F11;
        case GLFW_KEY_F12: return DOOM_KEY_F12;
        case GLFW_KEY_PAUSE: return DOOM_KEY_PAUSE;
        default: return DOOM_KEY_UNKNOWN;
    }
    return DOOM_KEY_UNKNOWN;
}


CK_DLL_CTOR(puredoom_ctor);
CK_DLL_DTOR(puredoom_dtor);

CK_DLL_MFUN(puredoom_update)
{
    doom_update();
}

CK_DLL_MFUN(puredoom_framebuffer)
{
    const unsigned char *framebuffer = doom_get_framebuffer(4);
    // hexDump("PureDOOM hexdump", framebuffer, 32);


    RETURN->v_int = (t_CKINT) framebuffer;

    /*
    int pitch = SCREENWIDTH * 4; // 4 channels per pixel
    int rows = SCREENHEIGHT;

    Chuck_ArrayInt *ck_arr = (Chuck_ArrayInt *)API->object->create(SHRED, ckint_array_type, false);

    for (int i = 0; i < pitch * rows; i += 4) {
        g_chuglAPI->object->array_int_push_back(ck_arr,
            framebuffer[i + 0] << (8 * 3) ||
            framebuffer[i + 1] << (8 * 2) ||
            framebuffer[i + 2] << (8 * 1) ||
            framebuffer[i + 3] << (8 * 0)
        );
    }

    RETURN->v_object = (Chuck_Object*) ck_arr;
    */
}


CK_DLL_MFUN(puredoom_key_down) { doom_key_down(chugl_keycode_to_doom_key(GET_NEXT_INT(ARGS))); }
CK_DLL_MFUN(puredoom_key_up) { doom_key_up(chugl_keycode_to_doom_key(GET_NEXT_INT(ARGS))); }
CK_DLL_MFUN(puredoom_button_down) { doom_button_down((doom_button_t) GET_NEXT_INT(ARGS)); }
CK_DLL_MFUN(puredoom_button_up) { doom_button_up((doom_button_t) GET_NEXT_INT(ARGS)); }
CK_DLL_MFUN(puredoom_mouse_move) {
    int dx = GET_NEXT_INT(ARGS);
    int dy = GET_NEXT_INT(ARGS);
    doom_mouse_move(dx, dy);
}

CK_DLL_TICKF(puredoom_tick);

// this is a special offset reserved for chugin internal data
t_CKINT puredoom_data_offset = 0;

CK_DLL_INFO(PureDOOM)
{
    // the version string of this chugin, e.g., "v1.2.1"
    QUERY->setinfo(QUERY, CHUGIN_INFO_CHUGIN_VERSION, "");
    // the author(s) of this chugin, e.g., "Alice Baker & Carl Donut"
    QUERY->setinfo(QUERY, CHUGIN_INFO_AUTHORS, "");
    // text description of this chugin; what is it? what does it do? who is it for?
    QUERY->setinfo(QUERY, CHUGIN_INFO_DESCRIPTION, "");
    // (optional) URL of the homepage for this chugin
    QUERY->setinfo(QUERY, CHUGIN_INFO_URL, "");
    // (optional) contact email
    QUERY->setinfo(QUERY, CHUGIN_INFO_EMAIL, "");
}

CK_DLL_QUERY(PureDOOM)
{
    g_chuglVM = QUERY->ck_vm(QUERY);
    g_chuglAPI = QUERY->ck_api(QUERY);
    ckint_array_type = g_chuglAPI->type->lookup(g_chuglVM, "int[]");

    QUERY->setname(QUERY, "PureDOOM");

    BEGIN_CLASS("PureDOOM", "UGen");
    puredoom_data_offset = MVAR("int", "@pdoom_data", false);

    static t_CKINT puredoom_screen_w = SCREENWIDTH;
    static t_CKINT puredoom_screen_h = SCREENHEIGHT;
    SVAR("int", "screenWidth", &puredoom_screen_w)
    SVAR("int", "screenHeight", &puredoom_screen_h)

    static t_CKINT puredoom_mouse_left = DOOM_LEFT_BUTTON;
    static t_CKINT puredoom_mouse_right = DOOM_RIGHT_BUTTON;
    static t_CKINT puredoom_mouse_middle = DOOM_MIDDLE_BUTTON;
    SVAR("int", "Button_MouseLeft", &puredoom_mouse_left);
    SVAR("int", "Button_MouseRight", &puredoom_mouse_right);
    SVAR("int", "Button_MouseMiddle", &puredoom_mouse_middle);

    CTOR(puredoom_ctor);
    DTOR(puredoom_dtor);

    QUERY->add_ugen_funcf(QUERY, puredoom_tick, NULL,
                            0, // 0 channels in
                            2  // stereo out
    );

    MFUN(puredoom_update, "void", "update");

    // possibly consolidate w/ update
    MFUN(puredoom_framebuffer, "int", "framebuffer");

    // input
    MFUN(puredoom_key_down, "void", "keyDown");
    ARG("int", "key");

    MFUN(puredoom_key_up, "void", "keyUp");
    ARG("int", "key");

    MFUN(puredoom_button_down, "void", "mouseDown");
    ARG("int", "key");

    MFUN(puredoom_button_up, "void", "mouseUp");
    ARG("int", "key");

    MFUN(puredoom_mouse_move, "void", "mouseMove");
    ARG("int", "dx");
    ARG("int", "dy");

    END_CLASS();

    return TRUE;
}

// implementation for the default constructor
CK_DLL_CTOR(puredoom_ctor)
{
    // PureDOOM * pdoom_obj = new PureDOOM( API->vm->srate(VM) );
    // OBJ_MEMBER_INT( SELF, puredoom_data_offset ) = (t_CKINT)pdoom_obj;
    // Change default bindings to modern
    doom_set_default_int("key_up", DOOM_KEY_W);
    doom_set_default_int("key_down", DOOM_KEY_S);
    doom_set_default_int("key_strafeleft", DOOM_KEY_A);
    doom_set_default_int("key_straferight", DOOM_KEY_D);
    doom_set_default_int("key_use", DOOM_KEY_E);
    doom_set_default_int("mouse_move", 0); // Mouse will not move forward
    doom_init(0, NULL, DOOM_FLAG_MENU_DARKEN_BG);
}

// implementation for the destructor
CK_DLL_DTOR(puredoom_dtor)
{
    // PureDOOM *pdoom_obj = (PureDOOM *)OBJ_MEMBER_INT(SELF, puredoom_data_offset);
    // CK_SAFE_DELETE(pdoom_obj);
    // OBJ_MEMBER_INT(SELF, puredoom_data_offset) = 0;
}

// implementation for tick function (relevant only for UGens)

int16_t audio_buffer[512 * 2] = {};
int audio_buffer_pos = 512;
uint64_t samp = 0;

CK_DLL_TICKF(puredoom_tick)
{
    // assume srate is 44100, only step every 1 samples because 
    // DOOM is sampled at 11025
    bool every_fourth = ((samp++ & 0b0011) == 0);
    if (every_fourth) ++audio_buffer_pos;

    if (audio_buffer_pos >= 512) {
        // reached end of buffer, request next
        memcpy(audio_buffer, doom_get_sound_buffer(), sizeof(audio_buffer)); 
        audio_buffer_pos = 0;
    }

    out[0] = audio_buffer[audio_buffer_pos*2 + 0] / (float) INT16_MAX;
    out[1] = audio_buffer[audio_buffer_pos*2 + 1] / (float) INT16_MAX;

    // I don't understand how the doom midi works... where are the instruments??
// while (midi_msg = doom_tick_midi())
//         midiOutShortMsg(midi_out_handle, midi_msg);

    return TRUE;
}

// example implementation for setter
CK_DLL_MFUN(puredoom_setParam)
{
}

// example implementation for getter
CK_DLL_MFUN(puredoom_getParam)
{
}
