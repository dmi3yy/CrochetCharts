if(WIN32)

    set(WIN32_BASE "C:/Qt/2010.05/qt")

    #generate the embeded icon for the exe file.
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
    set(crochet_win ${CMAKE_CURRENT_SOURCE_DIR}/resources/win/crochet.rc)

    set(WIN32_LIBS "${WIN32_BASE}/bin")
    set(WIN32_PLUGINS "${WIN32_BASE}/plugins")

    set(QT_LIBS_WINDOWS "${WIN32_LIBS}/QtSvg4.dll" "${WIN32_LIBS}/QtCore4.dll" "${WIN32_LIBS}/QtGui4.dll"
                        "${WIN32_LIBS}/QtNetwork4.dll" "${WIN32_LIBS}/QtXml4.dll")
    set(QT_DEPS_WINDOWS "${WIN32_LIBS}/libgcc_s_dw2-1.dll" "${WIN32_LIBS}/mingwm10.dll"
                        "${CMAKE_CURRENT_SOURCE_DIR}/resources/qt.conf" "C:/OpenSSL-Win32/libeay32.dll"
                        "C:/OpenSSL-Win32/ssleay32.dll")

    set(QT_PLUGINS_WINDOWS "${WIN32_PLUGINS}/imageformats" "${WIN32_PLUGINS}/accessible" "${WIN32_PLUGINS}/iconengines")

elseif (APPLE)
   
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" "[Paths]\nPlugins = plugins\n")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qt.conf" DESTINATION .)

    set(CMAKE_OSX_ARCHITECTURES "i386;x86_64") #";x86_64;ppc64;")

    set(DARWIN_LIBS "/Developer/SDKs/MacOSX10.4u.sdk/Library/Frameworks/")
    set(DARWIN_PLUGINS "/Developer/Applications/Qt/plugins")

    set(crochet_mac)
	
else() #LINUX
    set(crochet_nix)

endif()
