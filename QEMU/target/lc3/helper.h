/*
 * QEMU AVR CPU helpers
 *
 * Copyright (c) 2016-2020 Michael Rolnik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

DEF_HELPER_1(print_reg, void, env)
DEF_HELPER_1(print, void, i32)
DEF_HELPER_1(convert, tl, i32)
DEF_HELPER_2(st, void, i32, i32)
DEF_HELPER_2(key, void, env, i32)
DEF_HELPER_1(debug, noreturn, env)
DEF_HELPER_1(break, noreturn, env)
DEF_HELPER_1(sleep, noreturn, env)
DEF_HELPER_1(unsupported, noreturn, env)
DEF_HELPER_3(outb, void, env, i32, i32)
DEF_HELPER_2(inb, tl, env, i32)
DEF_HELPER_3(fullwr, void, env, i32, i32)
DEF_HELPER_2(fullrd, tl, env, i32)
DEF_HELPER_2(update_flags, void, env, i32)
DEF_HELPER_2(PUTS, void, env, i32)
DEF_HELPER_2(PUTSP, void, env, i32)
DEF_HELPER_2(OUT, void, env, i32)
DEF_HELPER_1(GETC, void, env)
DEF_HELPER_1(IN, void, env)
DEF_HELPER_1(HALT, void, env)