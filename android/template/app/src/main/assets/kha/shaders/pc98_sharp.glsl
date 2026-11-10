// pc98_sharp.glsl — PC-98 미니 에뮬 기본 셰이더 (비율 유지 전체화면용)
// 원본: sharp-shimmerless by zadpos (Public domain / CC0-1.0)
//   https://github.com/Woohyun-Kang/Sharp-Shimmerless-Shader  @2a74e7af599335c2ce5e493f823a757925c80db3
// 변경: RetroArch 규약(MVPMatrix·TextureSize·InputSize·OutputSize)을 우리 규약으로 평탄화. 알고리즘 동일.
//
// 호스트 약속
//  - 한 파일에 정점/조각 둘 다. 호스트가 맨 앞에 버전 줄과 "#define VERTEX" 또는 "#define FRAGMENT"를 붙여 컴파일.
//      GL 2.1: "#version 120" / GLES2: "#version 100" / GL3.x·GLES3: "#version 130" 또는 "#version 300 es" 모두 됨.
//  - attribute aPos(vec2, -1..1 화면 사각형), aUV(vec2, 0..1. 위=0)
//  - uniform uTex(sampler2D: 게임 화면 원본, 640x400 그대로 올린 텍스처, 필터 GL_LINEAR 필수, CLAMP_TO_EDGE)
//            uSrcSize(vec2: 원본 화소 수, 예 640,400)  uOutSize(vec2: 이 사각형이 차지하는 실제 화면 화소 수, 예 1440,1080)
//  - 4:3 비율 유지 = 호스트가 사각형(뷰포트)을 4:3 으로 잡는다. 셰이더는 비율을 모른다(가로·세로 배율이 달라도 정확).
//  - 정수배일 때는 결과가 nearest 와 같다(호스트는 정수배면 셰이더 없이 GL_NEAREST 로 그려도 됨).

#ifdef GL_ES
  #ifdef GL_FRAGMENT_PRECISION_HIGH
    precision highp float;   // 1024px 넘는 출력은 highp 필요(mediump 이면 경계가 1px 틀어짐)
  #else
    precision mediump float;
  #endif
#endif

#if __VERSION__ >= 130
  #ifdef VERTEX
    #define VARY out
    #define ATTR in
  #else
    #define VARY in
    out vec4 FragColor;
  #endif
  #define TEX texture
#else
  #define VARY varying
  #define ATTR attribute
  #define FragColor gl_FragColor
  #define TEX texture2D
#endif


uniform vec2 uSrcSize;
uniform vec2 uOutSize;

VARY vec2 vPix;      // 출력 화면 화소 좌표
VARY vec2 vScale;    // 출력/원본 배율
VARY vec2 vInv;      // 원본/출력 배율

#ifdef VERTEX
ATTR vec2 aPos;
ATTR vec2 aUV;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vPix   = aUV * uOutSize;
    vScale = uOutSize / uSrcSize;
    vInv   = uSrcSize / uOutSize;
}
#endif

#ifdef FRAGMENT
uniform sampler2D uTex;
void main()
{
    // 출력 화소 하나를 사각형으로 보고, 그 안에 원본 화소가 차지하는 면적만큼 섞는다(경계 화소만 섞임 → 흔들림 없음).
    vec2 p_tl = floor(vPix);
    vec2 p_br = ceil(vPix);
    vec2 t_tl = floor(vInv * p_tl);
    vec2 t_br = floor(vInv * p_br);
    vec2 t = t_br + vec2(0.5);
    t -= (vec2(1.0) - step(t_br, t_tl)) * (vScale * t_br - p_tl);
    FragColor = vec4(TEX(uTex, t / uSrcSize).rgb, 1.0);
}
#endif
