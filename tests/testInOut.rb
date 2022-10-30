# -*- coding: utf-8 -*-
require "minitest/autorun"
require "expect"
require "pty"

require "../tests/testConstantes"

class Test2InOut < Minitest::Test
  test_order=:defined

  def setup
    @pty_read, @pty_write = PTY.open
    @pipe_read, @pipe_write = IO.pipe
    @pid = spawn(COMMANDESHELL, :in=>@pipe_read, :out=>@pty_write)
    @pipe_read.close
    @pty_write.close
  end

  def teardown
    # nettoyer les fichiers de tests
    system("rm -f totoExpect.txt titiExpect.txt")
    @pty_read.close
    @pipe_write.close
  end

  def test_inout
    @pipe_write.puts("ls > totoExpect.txt")
    @pipe_write.puts("ls totoExpect.txt")
    a = @pty_read.expect(/totoExpect.txt/, DELAI)
    refute_nil(a, "sortie incohérente pour ls")
    @pipe_write.puts("ls -l totoExpect.txt")
    a = @pty_read.expect(/-r.*totoExpect.txt/, DELAI)
    refute_nil(a, "sortie incohérente pour ls -l")
    @pipe_write.puts("wc -l totoExpect.txt")
    a = @pty_read.expect(/^(\d+) .*\r\n/, DELAI)
    refute_nil(a, "sortie incohérente pour wc -l")
    nbfichier = a[1].to_i
    @pipe_write.puts("wc -l < totoExpect.txt")
    a = @pty_read.expect(/^(\d+)\r\n/, DELAI)
    refute_nil(a, "sortie incohérente pour wc -l")
    nbfichier2 = a[1].to_i
    assert_equal(nbfichier, nbfichier2, "wc -l ne compte pas le même nombre de ligne suivant qu'il utilise son entrée standard ou pas")
  end

    def test_pipe
      @pipe_write.puts("ls | wc -l")
      a = @pty_read.expect(/^(\d+)\r\n/, DELAI)
      refute_nil(a, "sortie incohérente pour ls | wc -l")
    end

    def test_inoutpipe
      @pipe_write.puts("ls > totoExpect.txt")
      @pipe_write.puts("cat < totoExpect.txt | wc -l > titiExpect.txt")
      @pipe_write.puts("cat titiExpect.txt titiExpect.txt")
      a = @pty_read.expect(/^(\d+)\r\n\1\r\n/, DELAI)
      refute_nil(a, "cat titiExpect.txt titiExpect.txt n'a pas rendu deux fois le même entier à la suite")
      nbfichierpipe= a[1].to_i
      @pipe_write.puts("wc -l totoExpect.txt")
      a = @pty_read.expect(/^(\d+) totoExpect.txt\r\n/, DELAI)
      refute_nil(a, "wc -l totoExpect.txt n'a pas rendu un entier")
      nbfichier= a[1].to_i
      puts "pipe "+nbfichier.to_s+" "+nbfichierpipe.to_s
      assert_equal(nbfichierpipe, nbfichier, "le nombre de fichier n'est pas le même suivant que la liste est passé dans votre pipe+redirection ou pas")
    end

    def test_parallelisme
      @pipe_write.puts("time -p sleep 3 | echo toto")
      a = @pty_read.expect(/^toto\r\n/, DELAI)
      refute_nil(a, "le | ne semble pas parallèle")
    end 


end
