# ============================================================================ #
# Copyright (c) 2022 - 2024 NVIDIA Corporation & Affiliates.                   #
# All rights reserved.                                                         #
#                                                                              #
# This source code and the accompanying materials are made available under     #
# the terms of the Apache License 2.0 which accompanies this distribution.     #
# ============================================================================ #

# RUN: PYTHONPATH=../../ pytest -rP  %s | FileCheck %s

import os

import pytest
import numpy as np

import cudaq


def test_bad_arg_checking_fix_1130():
    kernel, a, b, c = cudaq.make_kernel(cudaq.qubit, cudaq.qubit, float)
    kernel.cx(a, b)
    kernel.rz(2 * c, b)
    kernel.cx(a, b)

    qernel, theta = cudaq.make_kernel(float)
    qubits = qernel.qalloc(2)
    qernel.apply_call(kernel, qubits[0], qubits[1], theta)
    print(qernel)


# CHECK-LABEL:   func.func @__nvqpp__mlirgen____nvqppBuilderKernel_093606261879(
# CHECK-SAME:                                                                   %[[VAL_0:.*]]: f64) attributes {"cudaq-entrypoint"} {
# CHECK:           %[[VAL_1:.*]] = quake.alloca !quake.veq<2>
# CHECK:           %[[VAL_2:.*]] = quake.extract_ref %[[VAL_1]][0] : (!quake.veq<2>) -> !quake.ref
# CHECK:           %[[VAL_3:.*]] = quake.extract_ref %[[VAL_1]][1] : (!quake.veq<2>) -> !quake.ref
# CHECK:           call @__nvqpp__mlirgen____nvqppBuilderKernel_367535629127(%[[VAL_2]], %[[VAL_3]], %[[VAL_0]]) : (!quake.ref, !quake.ref, f64) -> ()
# CHECK:           return
# CHECK:         }

# CHECK:      %[[VAL_0:.*]] = arith.constant 2.000000e+00 : f64
# CHECK:           quake.x {{\[}}%[[VAL_1:.*]]] %[[VAL_2:.*]] : (!quake.ref, !quake.ref) -> ()
# CHECK:           %[[VAL_3:.*]] = arith.mulf %[[VAL_4:.*]], %[[VAL_0]] : f64
# CHECK:           quake.rz (%[[VAL_3]]) %[[VAL_2]] : (f64, !quake.ref) -> ()
# CHECK:           quake.x {{\[}}%[[VAL_1]]] %[[VAL_2]] : (!quake.ref, !quake.ref) -> ()
# CHECK:           return
