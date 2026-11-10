# KyuhachiX frontend setup (ES-DE / Daijisho)

Package: `io.github.ataho99.kyuhachix`
Activity: `org.libsdl.app.KhActivity` (launchMode singleInstance)

How it works: the frontend sends a VIEW intent with the game file
in `-d`. The app writes `"--gamefile" "<path>"` to `next_args.txt`
and the native side takes it as argv. The file stem (basename,
extension cut, case-insensitive) is matched against the scanned
game DB (game id, folder name, disk file names). On match the game
boots directly (menu skipped). On mismatch the menu opens, no error
box. A second call while a game runs is ignored (one log line).

## ES-DE (raw path, recommended)

Checked 2026-11-10 on a device (ES-DE `org.es_de.frontend`): 동급생2 boots directly.

1. `ES-DE/custom_systems/es_find_rules.xml`:

        <ruleList>
          <emulator name="KYUHACHIX">
            <rule type="androidpackage">
              <entry>io.github.ataho99.kyuhachix/org.libsdl.app.KhActivity</entry>
            </rule>
          </emulator>
        </ruleList>

2. `ES-DE/custom_systems/es_systems.xml` (system name `pc98` is an example;
   `<path>` is the ROM folder, here the Kyuhachi games folder):

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

3. Restart ES-DE (or reload custom systems). The PC98 system appears; start a
   game with the A button.

`%DATA%=%ROM%` expands to the raw path (no scheme). `%ROMRAW%` is NOT expanded
in this ES-DE build: the literal text is passed and the Kyuhachi menu opens
instead of the game. `%ROMSAF%` (content URI) was not tried.

Note: with the folder `.../Kyuhachi/games`, game disks under `saves/<id>/disk/`
are listed too; a file in a folder named `disk` is matched by its file name
only (see Notes).

## Daijisho (Add a Player)

Checked 2026-11-10 on a device (Daijisho 1.8.1, Korean UI). Daijisho builds its
platform list from a downloaded index, so there is no empty "add platform":

1. Settings (설정) > Library (라이브러리) > Add platform (플랫폼 추가): download
   the platform index and tick *NEC PC-98* (122 items offered).
2. Add the ROM folder through the system file picker (SAF), here
   `Kyuhachi/games`, then sync. Each disk file becomes its own item
   (16 items for the test set); a game with several disks asks which one to play.
3. Settings > Library > Add player (플레이어 추가). Fields:
   - Name: `Kyuhachi X`
   - Start arguments:

         -n io.github.ataho99.kyuhachix/org.libsdl.app.KhActivity
         -a android.intent.action.VIEW
         -d {file.path}
         --activity-clear-task
         --activity-clear-top

     (`{file.uri}` for the SAF form; not tried.)
   - Filename regex: `^(.*)[.](?:d88|fdi|hdi|hdm|dup|xdf)$`
   - "Kill package process before start": off.
4. The player is NOT linked to the platform automatically (the "6 players"
   shown are the RetroArch cores). Link it by hand: on the home screen
   highlight the platform, press the pencil (edit platform), scroll down.
   - "Search additional players" (추가할 추가 플레이어 검색): pick `0 - Kyuhachi X`
     (entries are listed as `N - name`; typing a name does not filter) and press
     "Add to list" (리스트에 추가).
   - "Default player" (기본 플레이어): choose `Kyuhachi X`.
   - Press Save (저장). Going back instead of saving discards the change.
5. Now **Play** on any item of the platform calls Kyuhachi X. Checked
   2026-11-10 on a device: 동급생2 boots directly. `{file.path}` is resolved
   by Daijisho to the raw path (`/storage/emulated/0/Kyuhachi/games/<id>/<file>`)
   even though the item itself is stored as a content URI.

Per-item "Edit item" has no player choice; the platform default player is the
way. Daijisho shows its "kill package process" prompt only for the default
player.

## Cocoon (Cocoon Shell)

Not supported. Cocoon's player list comes from the app's fixed remote
catalogue, so users cannot add their own player. Use ES-DE or Daijisho (above),
which send the same intent.

## Notes

- `content://` needs the sender to grant read permission. Without
  it the call is logged and ignored; use the raw-path form instead.
- ES-DE and Daijisho raw-path boot are verified on a device (2026-11-10).
  The content-URI permission form is unverified.
- Matching (v0.6.7) runs in three passes, in this order: game id (file stem or
  parent folder name) > disk file name / full path > game folder name. Generic
  folder names (`disk`, `disks`, `game`, `games`, `hdd`, `hd`, `fd`, `fdd`,
  `floppy`, `roms`, `rom`, `pc98`, `pc-98`, `saves`, `data`, `image`, `images`)
  are never used as a key, so `saves/nadia/disk/...hdi` finds the game by its
  disk file instead of a game named `disk`. The log line
  `GAMEFILE match pass=N` shows which pass matched. `--gamefile` also parses on
  Windows but that path is unverified.

(요약) ES-DE·Daijisho에서 VIEW 인텐트로 게임 파일을 넘기면 작품 id → 디스크 파일 → 폴더 이름 순으로 작품을 찾아 바로 실행하고, 못 찾으면 메뉴를 연다. 실행 중 재호출은 무시한다.
content:// 는 보낸 쪽이 읽기 권한을 줘야 하고 아니면 무시되므로, 실제 경로(raw path) 설정을 쓴다. 한국어 전체 설명: [frontends.ko.md](frontends.ko.md)
