# 큐하치 X 프런트엔드 연동 (ES-DE · Daijisho)

패키지: `io.github.ataho99.kyuhachix`
액티비티: `org.libsdl.app.KhActivity` (launchMode singleInstance)

## 동작 원리

프런트엔드가 게임 파일 경로를 담은 VIEW 인텐트(`-d`)를 보내면, 앱이 `next_args.txt` 에
`"--gamefile" "<경로>"` 를 적고 네이티브 쪽이 그것을 인자로 받습니다. 그러면 큐하치 X 가
메뉴를 건너뛰고 해당 작품을 바로 부팅합니다.

작품은 이 순서로 찾습니다.

1. **작품 id** — 파일 이름(확장자 제외, 대소문자 무시) 또는 바로 위 폴더 이름이 작품 id 와 같을 때
2. **디스크 파일** — 작품의 디스크 교체 목록과 프로필 A~D 이미지에서 파일 이름 또는 전체 경로가 같을 때
3. **작품 폴더 이름** — 파일 이름 또는 바로 위 폴더 이름이 작품 폴더 이름과 같을 때

`disk`, `disks`, `game`, `games`, `hdd`, `hd`, `fd`, `fdd`, `floppy`, `roms`, `rom`, `pc98`, `pc-98`,
`saves`, `data`, `image`, `images` 같은 일반 폴더 이름은 열쇠로 쓰지 않습니다. 그래서
`saves/nadia/disk/….hdi` 는 `disk` 라는 작품이 아니라 그 디스크 파일을 가진 작품(nadia)으로 열립니다.
맞는 작품이 없으면 오류 상자 없이 메뉴가 열립니다. 게임이 실행 중일 때 다시 호출하면 무시하고
로그에 한 줄만 남깁니다. 어느 단계에서 맞았는지는 로그의 `GAMEFILE match pass=N` 으로 알 수 있습니다.

## ES-DE (실제 경로 방식, 권장)

2026-11-10 기기에서 확인(ES-DE `org.es_de.frontend`): 동급생2 가 바로 부팅됩니다.

1. `ES-DE/custom_systems/es_find_rules.xml`:

        <ruleList>
          <emulator name="KYUHACHIX">
            <rule type="androidpackage">
              <entry>io.github.ataho99.kyuhachix/org.libsdl.app.KhActivity</entry>
            </rule>
          </emulator>
        </ruleList>

2. `ES-DE/custom_systems/es_systems.xml` (시스템 이름 `pc98` 은 예시이고, `<path>` 는 ROM 폴더, 여기서는
   큐하치 게임 폴더):

        <systemList>
          <system>
            <name>pc98</name>
            <fullname>NEC PC-98 (Kyuhachi X)</fullname>
            <path>/storage/emulated/0/Kyuhachi/games</path>
            <extension>.d88 .D88 .fdi .FDI .hdi .HDI .hdm .HDM .dup .DUP .xdf .XDF</extension>
            <command label="Kyuhachi X (raw path)">%EMULATOR_KYUHACHIX% %ACTIVITY_CLEAR_TASK% %ACTIVITY_CLEAR_TOP% %ACTION%=android.intent.action.VIEW %DATA%=%ROM%</command>
            <platform>pc98</platform>
            <theme>pc98</theme>
          </system>
        </systemList>

3. ES-DE 를 다시 시작하거나 사용자 정의 시스템을 다시 불러옵니다. PC98 시스템이 나타나면 게임에서
   A 버튼으로 시작합니다.

`%DATA%=%ROM%` 은 접두어 없는 실제 경로로 풀립니다. `%ROMRAW%` 는 이 ES-DE 빌드에서 풀리지 않고 글자 그대로
넘어가 게임 대신 큐하치 메뉴가 열립니다. `%ROMSAF%`(content URI)는 시험하지 않았습니다.

참고: 폴더를 `…/Kyuhachi/games` 로 잡으면 `saves/<id>/disk/` 아래 디스크도 목록에 함께 나옵니다. 위의 일반
폴더 이름 규칙 덕분에 그런 파일도 제 작품으로 열립니다.

## Daijisho (플레이어 추가)

2026-11-10 기기에서 확인(Daijisho 1.8.1, 한국어 화면): 동급생2 가 바로 부팅됩니다. Daijisho 는 내려받은
목록에서 플랫폼을 만들기 때문에 "빈 플랫폼 추가" 는 없습니다.

1. **설정 > 라이브러리 > 플랫폼 추가** 에서 플랫폼 목록을 내려받아 *NEC PC-98* 에 체크합니다.
2. 시스템 파일 선택창(SAF)으로 ROM 폴더(`Kyuhachi/games`)를 추가하고 동기화합니다. 디스크 파일마다
   항목이 하나씩 생기고, 디스크가 여러 장인 작품은 실행 때 어느 것을 쓸지 묻습니다.
3. **설정 > 라이브러리 > 플레이어 추가** 에서 다음을 채웁니다.
   - 이름: `Kyuhachi X`
   - 시작 인자:

         -n io.github.ataho99.kyuhachix/org.libsdl.app.KhActivity
         -a android.intent.action.VIEW
         -d {file.path}
         --activity-clear-task
         --activity-clear-top

     (`{file.uri}` 는 SAF 형식이며 시험하지 않았습니다.)
   - 파일 이름 정규식: `^(.*)[.](?:d88|fdi|hdi|hdm|dup|xdf)$`
   - "시작 전에 패키지 프로세스 종료": 끔
4. 추가한 플레이어는 플랫폼에 **자동으로 연결되지 않습니다**(플랫폼에 보이는 "플레이어 6" 은 RetroArch
   코어입니다). 직접 연결합니다. 홈 화면에서 플랫폼을 선택하고 연필(플랫폼 수정) 버튼을 눌러 아래로
   스크롤합니다.
   - **추가할 추가 플레이어 검색**: `0 - Kyuhachi X` 를 고르고 **리스트에 추가**를 누릅니다(항목이
     `번호 - 이름` 형식이라 이름을 쳐도 걸러지지 않습니다).
   - **기본 플레이어**: `Kyuhachi X` 를 고릅니다.
   - **저장**을 누릅니다. 저장하지 않고 뒤로 가면 바뀐 내용이 사라집니다.
5. 이제 플랫폼의 어느 항목이든 **플레이** 를 누르면 큐하치 X 가 열립니다. 항목은 content URI 로
   저장되어 있어도 `{file.path}` 는 Daijisho 가 실제 경로(`/storage/emulated/0/Kyuhachi/games/<id>/<파일>`)로
   바꿔 넘깁니다.

항목별 "아이템 수정" 에는 플레이어 선택이 없고, 플랫폼의 기본 플레이어로만 정합니다. "패키지 프로세스
종료" 경고는 기본 플레이어에서만 뜹니다.

## Cocoon (Cocoon Shell)

지원하지 않습니다. Cocoon 의 플레이어 목록은 앱의 고정된 원격 카탈로그에서 오기 때문에 사용자가 자기
플레이어를 추가할 수 없습니다. 같은 인텐트를 보내는 ES-DE 나 Daijisho 를 쓰세요.

## 참고

- `content://` 는 보낸 쪽이 읽기 권한을 줘야 합니다. 권한이 없으면 호출을 로그에 남기고 무시하니,
  실제 경로(raw path) 방식을 쓰세요.
- ES-DE 와 Daijisho 의 실제 경로 부팅은 기기에서 확인했습니다(2026-11-10). content URI 권한 방식은
  확인하지 못했습니다.
- `--gamefile` 은 윈도우에서도 해석되지만 그 경로는 확인하지 못했습니다.
