#!/bin/bash
# tools/wavdump_run.sh — 헤드리스로 같은 대본을 solo 설정만 바꿔 여러 번 돌려 장치별 WAV 를 모은다 (t832, #1010).
# 사용: wavdump_run.sh <run폴더> <게임id> <초> <inject파일|-> <출력폴더> [solo 목록 ...]
#   run폴더  = KyuhachiX.exe·res·settings.ini·strings_*.txt·gamedb*·games\<id>\game.ini 가 있는 작업 사본(원본 게임 폴더 아님). 동시에 두 개 돌리지 말 것(logs 충돌) — 사본을 따로 만든다.
#   solo 목록 = 쉼표 묶음 하나가 한 번의 실행. 예: "" fm ssg rhythm adpcm pcm86 beep cdda sb   ("" = 전부 켜고 한 번 → 채널 단위 WAV: PC-98·SPKR·CDAUDIO·SB…)
#   생략하면 "" fm ssg rhythm adpcm pcm86 cdda beep 를 차례로 돈다.
# 결과: <출력폴더>\<태그>_<채널>.wav · <태그>_summary.tsv · log_<태그>.txt(그 실행의 앱 로그) · all_summary.tsv(태그 붙여 합침)
# 주의: 대본(inject)이 같아도 에뮬 타이밍은 실행마다 조금 다를 수 있다 → 부품 비교는 「소리가 난 구간의 rms」 수준으로 읽고, 샘플 단위 정렬은 기대하지 않는다.
R=$(readlink -f "${1:?run폴더}"); G=${2:?게임id}; SEC=${3:?초}; INJ=${4:--}; O=${5:?출력폴더}; shift 5; [ "$INJ" != "-" ] && INJ=$(readlink -f "$INJ")   # t832: inject 는 절대경로로 고정
[ $# -gt 0 ] || set -- "" fm ssg rhythm adpcm pcm86 cdda beep
mkdir -p "$O"; O=$(readlink -f "$O"); cd "$R" || exit 2
OW=$(wslpath -w "$O")
: > "$O/all_summary.tsv"
for s in "$@"; do
  tag=${s:-all}; tag=${tag//,/+}
  args=(--headless --sec "$SEC" --wavdump "$OW" --wavsolo "$s")
  [ "$INJ" != "-" ] && args+=(--inject "$(wslpath -w "$INJ")")
  rm -f "logs/$G/kyuhachi_log.txt"
  timeout $((SEC + 60)) ./KyuhachiX.exe "${args[@]}" "$G" > "$O/out_$tag.txt" 2>&1; echo "rc=$?" >> "$O/out_$tag.txt"
  cp "logs/$G/kyuhachi_log.txt" "$O/log_$tag.txt" 2>/dev/null
  [ -f "$O/${tag}_summary.tsv" ] && sed "s/^/$tag\t/" "$O/${tag}_summary.tsv" | grep -v "^$tag	channel" >> "$O/all_summary.tsv"
  echo "run $tag done $(date +%H:%M:%S)"
done
echo "== all_summary (태그 채널 freq frames sec peak rms dbfs)"; cat "$O/all_summary.tsv"
