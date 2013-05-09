require 'formula'

class Kanabo < Formula
  homepage 'https://github.com/kevinbirch/kanabo'
  url 'https://github.com/kevinbirch/kanabo/archive/kanabo-0.1.1-SNAPSHOT.tar.gz'
  sha1 '483a6db0baaf5dc4baf5c8d2a056b4dd08e21412'

  depends_on 'check' => :build
  depends_on 'libyaml'

  def install
    system "make", "PREFIX=#{prefix}",
                   "CC=#{ENV.cc}",
                   "install"
  end
end
