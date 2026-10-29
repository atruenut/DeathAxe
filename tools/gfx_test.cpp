#include "../src/ui/gfx.h"
#include <cstdio>
using namespace dg::ui;
static void bmp(const Canvas& c, const char* path) {
    FILE* f = fopen(path, "wb"); const int rowb = c.w * 3, pad = (4 - rowb % 4) % 4, sz = 54 + (rowb + pad) * c.h;
    unsigned char h[54] = {'B','M'}; auto w32 = [&](int o, int v){ memcpy(h+o,&v,4); }; w32(2,sz); w32(10,54); w32(14,40); w32(18,c.w); w32(22,-c.h); h[26]=1; h[28]=24; w32(34,(rowb+pad)*c.h);
    fwrite(h,1,54,f); unsigned char z[4]={0};
    for (int y=0;y<c.h;++y){ for(int x=0;x<c.w;++x){ uint32_t p=c.px[y*c.w+x]; unsigned char b[3]={(unsigned char)p,(unsigned char)(p>>8),(unsigned char)(p>>16)}; fwrite(b,1,3,f);} fwrite(z,1,pad,f);} fclose(f);
}
int main(int argc, char** argv) {
    const float sc = argc > 1 ? (float)atof(argv[1]) : 1.0f;
    Canvas c; c.init((int)(520*sc), (int)(240*sc), sc); c.clear(rgb(0x0c0c0e));
    c.fillRoundRectV(10,10,160,100,8,rgb(0x222226),rgb(0x151517)); c.strokeRoundRect(10,10,160,100,8,1,rgb(0x3a3a40));
    c.fillCircle(60,60,24,rgb(0x2a2a2f));
    c.strokeArc(60,60,28,4,2.356f,2.356f+4.712f,rgb(0x2a2a2f));
    c.strokeArc(60,60,28,4,2.356f,2.356f+3.0f,rgb(0xd4202c),rgb(0xff7a1f),2.356f,4.712f);
    c.fillCircleV(60,60,19,rgb(0x4a4a50),rgb(0x1c1c1f)); c.line(60,60,70,45,2.5f,rgb(0xe8e3d8));
    float tri[6]={200,20,220,30,200,40}; c.fillConvex(tri,3,rgb(0xe8e3d8));
    c.text(kBold,28,200,90,"DEATHAXE",rgb(0xd4202c),kLeft,2.5f);
    c.text(kRegular,11,200,110,"Pick Attack  55 % -12 dB  4.0 s  \xC2\xB7 Mid Freq",rgb(0xe8e3d8));
    c.text(kRegular,9.5f,200,126,"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG 0123456789",rgb(0x8d8a84));
    c.text(kBold,10.5f,200,142,"AMP VOICING",rgb(0xe8e3d8),kLeft,1.2f);
    c.text(kRegular,11,260,170,"centered text",rgb(0xe8e3d8),kCenter);
    bmp(c,"build/gfx_test.bmp"); printf("ok %dx%d\n", c.w, c.h);
}
