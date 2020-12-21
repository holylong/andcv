// Copyright (C) 2016 Michael Ficarra. All rights reserved.// This code is governed by the BSD license found in the LICENSE file./*---esid: sec-function-definitions-runtime-semantics-instantiatefunctionobjectdescription: Function.prototype.toString line terminator normalisation (CR)info: |  Function.prototype.toString should not normalise line terminator sequences to Line Feed characters.  This file uses Carriage Return characters as line terminators.includes: [nativeFunctionMatcher.js]---*/// beforefunction// af// b(// cx// d,// ey// f)// g{// h;// i;// j}// afterassertToStringOrNativeFunction(f, "function\r// a\rf\r// b\r(\r// c\rx\r// d\r,\r// e\ry\r// f\r)\r// g\r{\r// h\r;\r// i\r;\r// j\r}");
