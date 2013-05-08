require 'formula'

class Kanabo < Formula
  homepage 'https://github.com/kevinbirch/kanabo'
  url 'https://github.com/kevinbirch/kanabo/archive/kanabo-0.1.0-SNAPSHOT.tar.gz'
  sha1 '4ab9e6696b760dd3abe834cbe36e77235629c7a8'

  depends_on 'check' => :build
  depends_on 'libyaml'

  def install
    system "make", "PREFIX=#{prefix}",
                   "CC=#{ENV.cc}",
                   "install"
  end
end
