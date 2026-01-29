#include "chugin.h"

#include <math.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint32_t u32;

#define PI 3.14159265358979

#define BB_MAX(x, y) (((x) > (y)) ? (x) : (y))
#define BB_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define BB_CLAMP(x, a, b) BB_MIN((BB_MAX(x, a)), b)

// range [0, 1]
// reaches max of 1 at x = 1/k
float expImpulse(float x, float k)
{
    float h = k * x;
    return h * exp(1.0 - h);
}

float music(int samp, float sr)
{
    float t = samp / sr; // time in seconds
    float u = t * 2;     // beat
    u = u - (int)u;      // clamp to [0,1]

    float f = 440;
    float o = sinf(t * 2 * PI * f);
    return expImpulse(u, 15) * o;
}

u8 triangle(u32 t) {
    return ((t>>5)%256 < 128) * (t>>4)
    |
    (1 - (((t>>5)%256 < 128))) * (-((t>>4) + 1));
}


// combines the last 4 8-bit samples into 1 u32
// in this order:
// 0b AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD
static u32 bytebeat_last = 0; 
u8 bytebeat(u32 t)
{
    // return t & (t >> 8);
    // return t & 0x01;
    // return (t * 9 & t >> 4 | t * 5 & t >> 7 | t * 3 & t >> 10); // pretty

    // return (
    //     t
    //     |
    //     t>>3
    //     |
    //     t>>5
    // );

    return (
        (17*t) 
        | 
        (t>>2) + ( t&32768 ? 13:14 )*t|t>>3|t>>5
    ) % 256 / 4;

    // sierpinski harmony
    // return t & (t >> 8);
}

CK_DLL_INFO(ByteBeat)
{
    QUERY->setinfo(QUERY, CHUGIN_INFO_CHUGIN_VERSION, "");
    QUERY->setinfo(QUERY, CHUGIN_INFO_AUTHORS, "");
    QUERY->setinfo(QUERY, CHUGIN_INFO_DESCRIPTION, "");
    QUERY->setinfo(QUERY, CHUGIN_INFO_URL, "");
    QUERY->setinfo(QUERY, CHUGIN_INFO_EMAIL, "");
}

static Chuck_VM *ck_vm = NULL;
static CK_DL_API ck_api = NULL;
static float ck_srate = 0;

CK_DLL_TICK(bytebeat_tick)
{
    t_CKTIME now = API->vm->now(ck_vm); // time in samps

    // floatbeat
    // *out = music((int)now, ck_srate);

    // bytebeat
    float sr_ratio = 8000.f / ck_srate;
    u32 t = sr_ratio * now;  // sample count at 8khz
    u8 bb = bytebeat(sr_ratio * now);

    // update bb last
    bytebeat_last &= (((u32) bb) << (8u * (t & 3u)));

    *out = bb / 255.0f * 2.0f - 1.0f;

    return 1; // returning 0 ignores the output
}

CK_DLL_MFUN(bytebeat_last_4_samps)
{
    RETURN->v_int = bytebeat_last;
}

CK_DLL_QUERY(ByteBeat)
{
    ck_vm = QUERY->ck_vm(QUERY);
    ck_api = (QUERY->ck_api(QUERY));
    ck_srate = (float)ck_api->vm->srate(ck_vm);

    // generally, don't change this...
    QUERY->setname(QUERY, "ByteBeat");

    QUERY->begin_class(QUERY, "ByteBeat", "UGen");

    // for UGens only: add tick function
    // NOTE a non-UGen class should remove or comment out this next line
    QUERY->add_ugen_func(
        QUERY, bytebeat_tick, NULL,
        0, // channels in
        1  // channels out
    );
    // NOTE: if this is to be a UGen with more than 1 channel,
    // e.g., a multichannel UGen -- will need to use add_ugen_funcf()
    // and declare a tickf function using CK_DLL_TICKF

    QUERY->add_mfun(QUERY, bytebeat_last_4_samps, "int", "bbLast4Samps");

    QUERY->end_class(QUERY);
    return 1;
}