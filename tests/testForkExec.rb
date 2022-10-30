#!/usr/bin/ruby
# -*- coding: utf-8 -*-

# Copyright (C) 2013 Gregory Mounie
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at
# your option) any later version.
 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require "minitest/autorun"
require "expect"
require "pty"

require "../tests/testConstantes"

class Test1ForkExec < Minitest::Test
  test_order=:defined

  def setup
    @pty_read, @pty_write = PTY.open
    @pipe_read, @pipe_write = IO.pipe
    @pid = spawn(COMMANDESHELL, :in=>@pipe_read, :out=>@pty_write)
    @pipe_read.close
    @pty_write.close
  end

  def teardown
    @pty_write.close
    @pipe_write.close
  end

  def test_seq
    @pipe_write.puts("seq 0 3")
    a = @pty_read.expect(/0\r\n1\r\n2\r\n3/m, DELAI)
    refute_nil(a, "Sortie incohérente pour 'seq 0 3'")
  end

  def test_printf
    @pipe_write.puts("printf 'toto%dtoto' 10")
    a = @pty_read.expect(/toto10toto/, DELAI)
    refute_nil(a, "Sortie incohérente pour 'seq 0 3'")
  end

  def test_all
    test_seq
    test_printf
  end
end
