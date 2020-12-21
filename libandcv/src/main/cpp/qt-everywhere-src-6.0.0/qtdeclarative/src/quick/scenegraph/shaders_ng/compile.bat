:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: Copyright (C) 2019 The Qt Company Ltd.
:: Contact: https://www.qt.io/licensing/
::
:: This file is part of the QtQuick module of the Qt Toolkit.
::
:: $QT_BEGIN_LICENSE:LGPL$
:: Commercial License Usage
:: Licensees holding valid commercial Qt licenses may use this file in
:: accordance with the commercial license agreement provided with the
:: Software or, alternatively, in accordance with the terms contained in
:: a written agreement between you and The Qt Company. For licensing terms
:: and conditions see https://www.qt.io/terms-conditions. For further
:: information use the contact form at https://www.qt.io/contact-us.
::
:: GNU Lesser General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU Lesser
:: General Public License version 3 as published by the Free Software
:: Foundation and appearing in the file LICENSE.LGPL3 included in the
:: packaging of this file. Please review the following information to
:: ensure the GNU Lesser General Public License version 3 requirements
:: will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
::
:: GNU General Public License Usage
:: Alternatively, this file may be used under the terms of the GNU
:: General Public License version 2.0 or (at your option) the GNU General
:: Public license version 3 or any later version approved by the KDE Free
:: Qt Foundation. The licenses are as published by the Free Software
:: Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
:: included in the packaging of this file. Please review the following
:: information to ensure the GNU General Public License requirements will
:: be met: https://www.gnu.org/licenses/gpl-2.0.html and
:: https://www.gnu.org/licenses/gpl-3.0.html.
::
:: $QT_END_LICENSE$
::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:: For HLSL we invoke fxc.exe (-c argument) and store the resulting intermediate format
:: instead of HLSL source, so this needs to be run on Windows from a developer command prompt.

:: For SPIR-V the optimizer is requested (-O argument) which means spirv-opt must be
:: invokable (e.g. because it's in the PATH from the Vulkan SDK)

qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o vertexcolor.vert.qsb vertexcolor.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o vertexcolor.frag.qsb vertexcolor.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o flatcolor.vert.qsb flatcolor.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o flatcolor.frag.qsb flatcolor.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o smoothcolor.vert.qsb smoothcolor.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o smoothcolor.frag.qsb smoothcolor.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o stencilclip.vert.qsb stencilclip.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o stencilclip.frag.qsb stencilclip.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o texture.vert.qsb texture.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o texture.frag.qsb texture.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o opaquetexture.vert.qsb opaquetexture.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o opaquetexture.frag.qsb opaquetexture.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o smoothtexture.vert.qsb smoothtexture.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o smoothtexture.frag.qsb smoothtexture.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o textmask.vert.qsb textmask.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o textmask.frag.qsb textmask.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o 8bittextmask.frag.qsb 8bittextmask.frag
qsb --glsl "150,120,100 es" -o 8bittextmask_a.frag.qsb 8bittextmask_a.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o 24bittextmask.frag.qsb 24bittextmask.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o 32bitcolortext.frag.qsb 32bitcolortext.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o outlinedtext.vert.qsb outlinedtext.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o outlinedtext.frag.qsb outlinedtext.frag
qsb --glsl "150,120,100 es" -o outlinedtext_a.frag.qsb outlinedtext_a.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o styledtext.vert.qsb styledtext.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o styledtext.frag.qsb styledtext.frag
qsb --glsl "150,120,100 es" -o styledtext_a.frag.qsb styledtext_a.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldtext.vert.qsb distancefieldtext.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldtext.frag.qsb distancefieldtext.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldtext_fwidth.frag.qsb distancefieldtext_fwidth.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldtext_a.frag.qsb distancefieldtext_a.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldtext_a_fwidth.frag.qsb distancefieldtext_a_fwidth.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldshiftedtext.vert.qsb distancefieldshiftedtext.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldshiftedtext.frag.qsb distancefieldshiftedtext.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldshiftedtext_fwidth.frag.qsb distancefieldshiftedtext_fwidth.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldshiftedtext_a.frag.qsb distancefieldshiftedtext_a.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldshiftedtext_a_fwidth.frag.qsb distancefieldshiftedtext_a_fwidth.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldoutlinetext.vert.qsb distancefieldoutlinetext.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldoutlinetext.frag.qsb distancefieldoutlinetext.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldoutlinetext_fwidth.frag.qsb distancefieldoutlinetext_fwidth.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldoutlinetext_a.frag.qsb distancefieldoutlinetext_a.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o distancefieldoutlinetext_a_fwidth.frag.qsb distancefieldoutlinetext_a_fwidth.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o hiqsubpixeldistancefieldtext.vert.qsb hiqsubpixeldistancefieldtext.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o hiqsubpixeldistancefieldtext.frag.qsb hiqsubpixeldistancefieldtext.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o hiqsubpixeldistancefieldtext_a.frag.qsb hiqsubpixeldistancefieldtext_a.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o loqsubpixeldistancefieldtext.vert.qsb loqsubpixeldistancefieldtext.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o loqsubpixeldistancefieldtext.frag.qsb loqsubpixeldistancefieldtext.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o loqsubpixeldistancefieldtext_a.frag.qsb loqsubpixeldistancefieldtext_a.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o shadereffect.vert.qsb shadereffect.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o shadereffect.frag.qsb shadereffect.frag
qsb -b --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o sprite.vert.qsb sprite.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o sprite.frag.qsb sprite.frag
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o visualization.vert.qsb visualization.vert
qsb --glsl "150,120,100 es" --hlsl 50 --msl 12 -O -c -o visualization.frag.qsb visualization.frag
