float_calc
==========

`float` (32ビット浮動小数点数) の計算をソフトウェアで行う。

## ビルド方法

1. gcc と make をインストールする (インストールされていない場合)
2. `make` コマンドを実行する

## 実行ファイルの処理内容

## float_test

いくつかの埋め込まれた値、およびコマンドラインで入力された値について、
標準の `float` の計算および実装した関数を用いてそれぞれ計算を行い、結果をビット単位で比較する。

`NaN` の中身が異なるために FAIL と判定される場合がある。

## float_test2

[fptest: IEEE754r floating point conformance tests](https://hackage.haskell.org/package/fptest)

で入手できるデータ (`test_suite`) を用いてテストを行う。

テストケースの構文の情報は

[Floating-Point Test-Suite for IEEE](http://web.archive.org/web/20220803044537/https://www.research.ibm.com/haifa/projects/verification/fpgen/papers/ieee-test-suite-v2.pdf)

にある。

オーバーフロー・アンダーフローの無視や、偶数丸め以外の丸めは実装していないため、
これらを用いるテストケースでは failed と判定されることがある。

また、inexact などのフラグの処理も実装しておらず、テストケースでの指定は無視する。
