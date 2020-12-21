// Copyright 2017 Mathias Bynens. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
author: Mathias Bynens
description: >
  Unicode property escapes for `Script=Old_Persian`
info: |
  Generated by https://github.com/mathiasbynens/unicode-property-escapes-tests
  Unicode v10.0.0
  Emoji v5.0 (UTR51)
esid: sec-static-semantics-unicodematchproperty-p
features: [regexp-unicode-property-escapes]
includes: [regExpUtils.js]
---*/

const matchSymbols = buildString({
  loneCodePoints: [],
  ranges: [
    [0x0103A0, 0x0103C3],
    [0x0103C8, 0x0103D5]
  ]
});
testPropertyEscapes(
  /^\p{Script=Old_Persian}+$/u,
  matchSymbols,
  "\\p{Script=Old_Persian}"
);
testPropertyEscapes(
  /^\p{Script=Xpeo}+$/u,
  matchSymbols,
  "\\p{Script=Xpeo}"
);
testPropertyEscapes(
  /^\p{sc=Old_Persian}+$/u,
  matchSymbols,
  "\\p{sc=Old_Persian}"
);
testPropertyEscapes(
  /^\p{sc=Xpeo}+$/u,
  matchSymbols,
  "\\p{sc=Xpeo}"
);

const nonMatchSymbols = buildString({
  loneCodePoints: [],
  ranges: [
    [0x00DC00, 0x00DFFF],
    [0x000000, 0x00DBFF],
    [0x00E000, 0x01039F],
    [0x0103C4, 0x0103C7],
    [0x0103D6, 0x10FFFF]
  ]
});
testPropertyEscapes(
  /^\P{Script=Old_Persian}+$/u,
  nonMatchSymbols,
  "\\P{Script=Old_Persian}"
);
testPropertyEscapes(
  /^\P{Script=Xpeo}+$/u,
  nonMatchSymbols,
  "\\P{Script=Xpeo}"
);
testPropertyEscapes(
  /^\P{sc=Old_Persian}+$/u,
  nonMatchSymbols,
  "\\P{sc=Old_Persian}"
);
testPropertyEscapes(
  /^\P{sc=Xpeo}+$/u,
  nonMatchSymbols,
  "\\P{sc=Xpeo}"
);
