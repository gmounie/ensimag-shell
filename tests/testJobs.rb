# -*- coding: utf-8 -*-
require "minitest/autorun"
require "expect"
require "pty"

require "../tests/testConstantes"

class Test3Jobs < Minitest::Test
  test_order=:defined

  def setup
    @pty_read, @pty_write = PTY.open
    @pipe_read, @pipe_write = IO.pipe
    @pid = spawn(COMMANDESHELL, :in=>@pipe_read, :out=>@pty_write)
    @pipe_read.close
    @pty_write.close
  end

  def teardown
    system("rm -f totoExpect.txt titiExpect.txt")
    @pty_read.close
    @pipe_write.close
  end

  def test_inout
    @pipe_write.puts("touch totoExpect.txt")
    @pipe_write.puts("sleep 10 &")
    @pipe_write.puts("ls -s totoExpect.txt")
    @pipe_write.puts("jobs")
    a = @pty_read.expect(/0 totoExpect.txt/, DELAI)
    refute_nil(a, "le ls n'a pas afficher la taille du fichier")
    a = @pty_read.expect(/sleep/, DELAI)
    refute_nil(a, "jobs n'affiche pas le nom de la commande sleep")
  end
end
