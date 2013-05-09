require 'formula'

class Kanabo < Formula
  homepage 'https://github.com/kevinbirch/kanabo'
  url 'https://github.com/kevinbirch/kanabo/archive/kanabo-0.1.1-SNAPSHOT.tar.gz'

  depends_on 'check' => :build
  depends_on 'libyaml'

  def install
    system "make", "PREFIX=#{prefix}",
                   "CC=#{ENV.cc}",
                   "install"
  end
end
