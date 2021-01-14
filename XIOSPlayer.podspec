#
#  Be sure to run `pod spec lint XIOSPlayer.podspec" to ensure this is a
#  valid spec and to remove all comments including this before submitting the spec.
#
#  To learn more about Podspec attributes see https://guides.cocoapods.org/syntax/podspec.html
#  To see working Podspecs in the CocoaPods repo see https://github.com/CocoaPods/Specs/
#

Pod::Spec.new do |spec|
  spec.name         = "XIOSPlayer"
  spec.version      = "0.0.1"
  spec.summary      = "iOS播放器"
  
  spec.description  = <<-DESC
  iOS播放器
  DESC
  
  spec.ios.deployment_target = "9.0"
  
  spec.homepage     = "https://github.com/oogh/XIOSPlayer.git"
  spec.author       = { "oogh" => "oogh216@163.com" }
  spec.source       = { :git => "https://github.com/oogh/XIOSPlayer.git", :tag => "#{spec.version}" }
  
  spec.subspec 'coremedia' do |coremedia|
    coremedia.source_files  = "coremedia/**/*.{h,hpp,cpp}"
    coremedia.exclude_files = "coremedia/Android", "coremedia/Mac"
    coremedia.public_header_files = "coremedia/**/*.{h,hpp}"
    coremedia.frameworks = "GLKit", "VideoToolBox", "CoreMedia", "OpenGLES", "OpenAL", "AVFoundation", "QuartzCore"
    coremedia.libraries = "c++", "iconv"
    
    coremedia.subspec 'iOS' do |iOS|
      iOS.source_files  = "coremedia/iOS/*.{h,hpp,cpp,m,mm}"
      iOS.public_header_files = "coremedia/iOS/*.{h,hpp}"
    end
  end
  
  spec.subspec '3rdparty' do |thirdparty|
    thirdparty.subspec 'ffmpeg' do |ffmpeg|
       ffmpeg.xcconfig = {"HEADER_SEARCH_PATHS" => '"$(PODS_TARGET_SRCROOT)/3rdparty/iOS/ffmpeg/include"'}
       ffmpeg.vendored_libraries  = '3rdparty/iOS/ffmpeg/lib/*.a'
       ffmpeg.libraries = 'z', 'bz2'
    end
    
    thirdparty.subspec 'fdkaac' do |fdkaac|
       fdkaac.xcconfig = {"HEADER_SEARCH_PATHS" => '"$(PODS_TARGET_SRCROOT)/3rdparty/iOS/fdkaac/include"'}
       fdkaac.vendored_libraries  = '3rdparty/iOS/fdkaac/lib/*.a'
    end
    
    thirdparty.subspec 'lame' do |lame|
       lame.xcconfig = {"HEADER_SEARCH_PATHS" => '"$(PODS_TARGET_SRCROOT)/3rdparty/iOS/lame/include"'}
       lame.vendored_libraries  = '3rdparty/iOS/lame/lib/*.a'
    end
    
    thirdparty.subspec 'x264' do |x264|
       x264.xcconfig = {"HEADER_SEARCH_PATHS" => '"$(PODS_TARGET_SRCROOT)/3rdparty/iOS/x264/include"'}
       x264.vendored_libraries  = '3rdparty/iOS/x264/lib/*.a'
    end
    
    thirdparty.subspec 'yuv' do |yuv|
       yuv.xcconfig = {"HEADER_SEARCH_PATHS" => '"$(PODS_TARGET_SRCROOT)/3rdparty/iOS/libyuv/include"'}
       yuv.vendored_libraries  = '3rdparty/iOS/libyuv/lib/*.a'
    end
    
    thirdparty.subspec 'sox' do |sox|
       sox.xcconfig = {"HEADER_SEARCH_PATHS" => '"$(PODS_TARGET_SRCROOT)/3rdparty/iOS/sox/include"'}
       sox.vendored_libraries  = '3rdparty/iOS/sox/lib/*.a'
    end
  end
end
