# check_launch_env_defaults.py

2026-08-12に発生した「提出したら成功表示なのに、システム上では一切実行された形跡がなく
ログも成果物も全部Access Denied」という提出失敗事故の再発防止ツール。ROS2 launch XML
ファイルを静的スキャンし、事故の原因になった書き方を検出する。

## 事故の原因(要約)

ROS2 launchの`$(env NAME default)`という置換構文は、`NAME`という環境変数が
**本当に未定義**の場合、`default`部分を**クオート等を一切解釈せず生テキストのまま**
代入する。

```xml
<!-- 「空文字列がデフォルト」のつもりで書いたコード -->
<arg name="foo" default="$(env FOO '')"/>
```

`FOO`が未定義のとき、これは空文字列にはならず、文字通り `''` という **2文字** が
そのまま`foo`の値になる。これを

```xml
<let name="bar" value="$(eval &quot;'$(env FOO '')' == 'true'&quot;)"/>
```

のように外側で`$(eval "...")`のPython式に埋め込んでいると、クオート文字が想定より
多く紛れ込み、`'''' == 'true'`のような不正な式になって

```
SyntaxError: unterminated triple-quoted string literal
```

でクラッシュする。このクラッシュはROSノードが1つも起動する前、launch記述の評価段階で
発生するため、**ログが一切出力されない**。

ローカル開発ではdocker-compose経由で起動することが多く、docker-composeの
`environment:`セクションが`FOO=${FOO:-}`のように「未設定でも空文字列として必ず定義」
する書き方をしていると、`FOO`は常に「存在する(中身が空)」状態になりこのバグは踏まない。
しかし本番の評価パイプラインはdocker-composeを経由しないことが多く、その場合`FOO`は
文字通り「存在しない」ため、バグが顕在化する。つまり **ローカルでは何度テストしても
再現せず、本番提出でだけ毎回失敗する** という厄介な性質を持つ。

## 使い方

```bash
python3 check_launch_env_defaults.py [パス...]
```

引数を省略するとカレントディレクトリを再帰的にスキャンする。ファイル・ディレクトリの
どちらも指定可能:

```bash
# 提出パッケージだけをチェック
python3 aichallenge/tools/submit_check_tool/check_launch_env_defaults.py \
    aichallenge/workspace/src/aichallenge_submit

# ワークスペース全体
python3 aichallenge/tools/submit_check_tool/check_launch_env_defaults.py \
    aichallenge/workspace/src

# 単一ファイル
python3 aichallenge/tools/submit_check_tool/check_launch_env_defaults.py \
    aichallenge/workspace/src/aichallenge_submit/aichallenge_submit_launch/launch/control/mpc.launch.xml
```

### オプション

| オプション | 説明 |
|---|---|
| `--ext .xml,.launch` | ディレクトリを指定したときにスキャンする拡張子(カンマ区切り、既定は`.xml`) |
| `--strict` | WARNINGレベルの検出があっても終了コードを1にする(既定はERRORのみ) |
| `--quiet` | 個別の検出結果を表示せず、最後のサマリー行だけ表示 |

### 終了コード

- `0`: 問題なし(`--strict`指定時はWARNINGも0件)
- `1`: ERROR(既定)、または`--strict`指定時はWARNINGも含めて1件以上検出

CIやpre-commitフックにそのまま組み込める。例(pre-commitフック的な使い方):

```bash
python3 aichallenge/tools/submit_check_tool/check_launch_env_defaults.py \
    aichallenge/workspace/src/aichallenge_submit || exit 1
```

## 検出内容

### ERROR: `$(env NAME default)` の`default`にクオート文字(`'`または`"`)が含まれる

事故の直接原因そのもの。`$(eval ...)`の中で使われているかどうかに関わらず検出する
(このバグは「今は使っていないが後でevalに組み込まれて初めて発火する」形で混入した
実績があるため、使用箇所を問わず即座に危険信号として扱う)。

検出時は原則としてクオートを含まない安全な語(例: `UNSET`)をデフォルトにし、
空/未設定判定は`$(env)`のフォールバックに頼らず、値を受け取った側で明示的に比較する
書き方を提案する:

```xml
<!-- Before(危険) -->
<arg name="foo_raw" default="$(env FOO '')"/>
<let name="bar" value="$(eval &quot;'$(var foo_raw)' == 'true'&quot;)"/>

<!-- After(安全) -->
<arg name="foo_raw" default="$(env FOO UNSET)"/>
<let name="bar" value="$(eval &quot;'$(var foo_raw)' not in ('UNSET', '')&quot;)"/>
```

### WARNING: `$(eval "...")` 内のシングルクオートの数が奇数(ヒューリスティック)

補助的なチェック。XMLに書かれた文字列としてクオートの数を数えているだけで、中の
`$(var ...)`/`$(env ...)`が実行時にどんな値を注入するかまでは追跡していないため、
見逃し・誤検知どちらもあり得る参考情報。**ERRORレベルの検出が本命であり、こちらは
「念のため目視確認した方がいい箇所」程度の位置づけ**。

## 動作確認

- 現在の`aichallenge_submit`配下(修正済みの状態)に対して実行し、35ファイルをスキャン
  してERROR/WARNINGともに0件であることを確認済み(2026-08-14時点)。

  ```
  $ python3 aichallenge/tools/submit_check_tool/check_launch_env_defaults.py \
        aichallenge/workspace/src/aichallenge_submit
  Scanned 35 file(s): 0 error(s), 0 warning(s).
  ```

  現ツリーに残っている`$(env ...)`は`$(env VEHICLE_ROLE racer)`,
  `$(env VEHICLE_ID d1)`(pure_pursuit.launch.xml)、
  `$(env USE_RECOVERY_SUPERVISOR true)`, `$(env USE_RECOVERY false)`
  (reference.launch.xml)の4種で、いずれもクオートを含まないデフォルトのため
  検出対象外。これが本ツールが維持したい状態そのもの。
- 事故発生当時の書き方を再現したサンプルファイルに対して実行し、該当2箇所
  (`MPC_OBSTACLE_SPEED_CAP_KMH`・`ENABLE_OVERTAKE_INDICATORS`相当)を正しく検出し、
  安全な書き方(`$(env NAME false)`など、クオートを含まないデフォルト)は誤検知しない
  ことを確認済み。
- XMLコメント内で「このパターンは危険」と説明文として言及しているだけの箇所は誤検知
  しないよう、`<!-- ... -->`をスキャン対象から除外する処理を入れている
  (`mask_xml_comments()`。改行を残して潰すため行番号もずれない)。

## 制限事項

- ROS2 launchの`$(...)`置換構文を厳密に(公式パーサーと同一の文法で)パースしている
  わけではなく、空白区切り・カッコの対応関係を見る簡易パーサーで近似している。極端に
  変則的な書き方(デフォルト値そのものに空白を含めるなど)では誤検知・見逃しの余地が
  ある。
- `.launch.py`(Python形式のlaunchファイル)は対象外。`EnvironmentVariable`
  substitutionを使ったPythonコード側で同種のミスをしていないかは別途目視確認が必要。
- あくまで「静的解析で見つかる典型パターン」を検出するツールであり、これを通せば
  安全と保証するものではない。可能であれば本番と同じビルド経路
  (`docker_build.sh eval --submit`)での実起動確認も引き続き推奨する。
