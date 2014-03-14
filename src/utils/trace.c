/*!The Treasure Box Library
 * 
 * TBox is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 * 
 * TBox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with TBox; 
 * If not, see <a href="http://www.gnu.org/licenses/"> http://www.gnu.org/licenses/</a>
 * 
 * Copyright (C) 2009 - 2015, ruki All rights reserved.
 *
 * @author		ruki
 * @file		trace.c
 * @ingroup 	utils
 *
 */

/* ///////////////////////////////////////////////////////////////////////
 * includes
 */
#include "trace.h"
#include "../libc/libc.h"
#include "../platform/platform.h"

/* ///////////////////////////////////////////////////////////////////////
 * macros
 */

// the trace line maxn
#ifdef __tb_small__
# 	define TB_TRACE_LINE_MAXN 		(4096)
#else
# 	define TB_TRACE_LINE_MAXN 		(8192)
#endif

/* ///////////////////////////////////////////////////////////////////////
 * globals
 */

// the mode
static tb_size_t 		g_mode = TB_TRACE_MODE_PRINT;

// the file
static tb_handle_t 		g_file = tb_null;

// the file is referenced?
static tb_bool_t 		g_bref = tb_false;

// the line
static tb_char_t 		g_line[TB_TRACE_LINE_MAXN];

// the lock
static tb_spinlock_t 	g_lock = TB_SPINLOCK_INIT; 

/* ///////////////////////////////////////////////////////////////////////
 * implementation
 */
tb_bool_t tb_trace_init()
{
	// init lock
	return tb_spinlock_init(&g_lock);
}
tb_void_t tb_trace_exit()
{
	// enter
	tb_spinlock_enter(&g_lock);

	// clear mode
	g_mode = TB_TRACE_MODE_PRINT;

	// clear file
	if (g_file && !g_bref) tb_file_exit(g_file);
	g_file = tb_null;
	g_bref = tb_false;

	// leave
	tb_spinlock_leave(&g_lock);

	// exit lock
	tb_spinlock_exit(&g_lock);
}
tb_size_t tb_trace_mode()
{
	// enter
	tb_spinlock_enter(&g_lock);

	// the mode
	tb_size_t mode = g_mode;

	// leave
	tb_spinlock_leave(&g_lock);

	// ok?
	return mode;
}
tb_bool_t tb_trace_mode_set(tb_size_t mode)
{
	// enter
	tb_spinlock_enter(&g_lock);

	// set the mode
	g_mode = mode;

	// leave
	tb_spinlock_leave(&g_lock);

	// ok
	return tb_true;
}
tb_handle_t tb_trace_file()
{
	// enter
	tb_spinlock_enter(&g_lock);

	// the file
	tb_handle_t file = g_file;

	// leave
	tb_spinlock_leave(&g_lock);

	// ok?
	return file;
}
tb_bool_t tb_trace_file_set(tb_handle_t file)
{
	// check
	tb_check_return_val(file, tb_false);

	// enter
	tb_spinlock_enter(&g_lock);

	// exit the previous file
	if (g_file && !g_bref) tb_file_exit(g_file);

	// set the file
	g_file = file;
	g_bref = tb_true;

	// leave
	tb_spinlock_leave(&g_lock);

	// ok
	return tb_true;
}
tb_bool_t tb_trace_file_set_path(tb_char_t const* path, tb_bool_t bappend)
{
	// check
	tb_check_return_val(path, tb_false);

	// enter
	tb_spinlock_enter(&g_lock);

	// exit the previous file
	if (g_file && !g_bref) tb_file_exit(g_file);

	// set the file
	g_file = tb_file_init(path, TB_FILE_MODE_RW | TB_FILE_MODE_BINARY | TB_FILE_MODE_CREAT | (bappend? TB_FILE_MODE_APPEND : TB_FILE_MODE_TRUNC));
	g_bref = tb_false;

	// ok?
	tb_bool_t ok = g_file? tb_true : tb_false;

	// leave
	tb_spinlock_leave(&g_lock);

	// ok?
	return ok;
}
tb_void_t tb_trace_done(tb_char_t const* prefix, tb_char_t const* module, tb_char_t const* format, ...)
{
	// check
	tb_check_return(format);

	// enter
	tb_spinlock_enter(&g_lock);

	// done
	do
	{
		// init
		tb_va_list_t 	l;
		tb_char_t* 		p = g_line;
		tb_char_t* 		e = g_line + sizeof(g_line);
		tb_va_start(l, format);

		// append prefix
		if (prefix && p < e) p += tb_snprintf(p, e - p, "[%s]: ", prefix);

		// append module
		if (module && p < e) p += tb_snprintf(p, e - p, "[%s]: ", module);

		// append format
		if (p < e) p += tb_vsnprintf(p, e - p, format, l);

		// append '\r'
#ifdef TB_CONFIG_OS_WINDOWS
		if (p < e) *p++ = '\r';
#endif

		// append '\n'
		if (p < e) *p++ = '\n';

		// append end
		if (p < e) *p = '\0'; e[-1] = '\0';

		// print it
		tb_print(g_line);

		// exit
		tb_va_end(l);

	} while (0);

	// leave
	tb_spinlock_leave(&g_lock);
}