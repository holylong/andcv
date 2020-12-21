#############################################################################
#
# Copyright (C) 2020 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of the test suite of the Qt Toolkit.
#
# $QT_BEGIN_LICENSE:GPL-EXCEPT$
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
# $QT_END_LICENSE$
#
#############################################################################


from option_management import need_separate_output_sequence


# ["hallo", "welt"] -> "halloWelt"
def build_camel_case(*args):
    assert all(isinstance(x, str) for x in args)
    if len(args) == 0:
        return ""
    if len(args) == 1:
        return args[0]
    return args[0] + "".join(x.capitalize() for x in args[1:])


def build_function_name(options):
    result = []
    for part in ["blocking", "filter", "map", "reduce"]:
        if options[part]:
            result.append(part)

    def make_potentially_passive(verb):
        if verb == "map":
            return "mapped"
        if verb == "blocking":
            return "blocking"

        if verb[-1] == "e":
            verb += "d"
        else:
            verb += "ed"

        return verb

    if not options["inplace"]:
        result = [make_potentially_passive(x) for x in result]

    result = build_camel_case(*result)
    return result


def build_blocking_return_type(options):
    if options["inplace"]:
        if options["filter"] and options["iterators"] and not options["reduce"]:
            return "Iterator"  # have to mark new end
        else:
            return "void"
    else:
        if options["reduce"]:
            return "ResultType"

        if need_separate_output_sequence(options):
            return "OutputSequence"
        else:
            return "Sequence"


def build_return_type(options):
    if options["blocking"]:
        return build_blocking_return_type(options)
    else:
        return f"QFuture<{build_blocking_return_type(options)}>"


def build_template_argument_list(options):
    required_types = []
    if options["reduce"]:
        required_types.append("typename ResultType")

    need_output_sequence = need_separate_output_sequence(options)
    if need_output_sequence:
        required_types.append("OutputSequence")

    if options["iterators"]:
        required_types.append("Iterator")
    else:
        if need_output_sequence:
            required_types.append("InputSequence")
        else:
            required_types.append("Sequence")

    # Functors
    if options["filter"]:
        required_types.append("KeepFunctor")

    if options["map"]:
        required_types.append("MapFunctor")

    if options["reduce"]:
        required_types.append("ReduceFunctor")

    if options["initialvalue"]:
        required_types.append("reductionitemtype")

    return "template<" + ", ".join(["typename "+x for x in required_types]) + ">"


def build_argument_list(options):
    required_arguments = []
    if options["pool"]:
        required_arguments.append("QThreadPool* pool")

    if options["iterators"]:
        required_arguments.append("Iterator begin")
        required_arguments.append("Iterator end")
    else:
        if options["inplace"]:
            required_arguments.append("Sequence & sequence")
        else:
            if need_separate_output_sequence(options):
                required_arguments.append("InputSequence && sequence")
            else:
                required_arguments.append("const Sequence & sequence")

    if options["filter"]:
        required_arguments.append("KeepFunctor filterFunction")

    if options["map"]:
        required_arguments.append("MapFunctor function")

    if options["reduce"]:
        required_arguments.append("ReduceFunctor reduceFunction")
        if options["initialvalue"]:
            required_arguments.append("reductionitemtype && initialValue")

        required_arguments.append("ReduceOptions")

    return "(" + ", ".join(required_arguments) + ")"


def build_function_signature(options):
    return (build_template_argument_list(options) + "\n" +
            build_return_type(
                options) + " " + build_function_name(options) + build_argument_list(options)
            + ";"
            )
