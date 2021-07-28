project(amlMpTestSupporter)

SET(AML_MP_TESTSUPPORTER_SRC
    AmlMpTestSupporter.cpp
    DVRPlayback.cpp
    DVRRecord.cpp
    DVRRecord.h
    ParserReceiver.cpp
    ParserReceiver.h
    Playback.cpp
    TestModule.cpp
    TestUtils.cpp
    source/Source.cpp
    source/DvbSource.cpp
    source/FileSource.cpp
    source/UdpSource.cpp
)

SET(TARGET amlMpTestSupporter)

ADD_LIBRARY(${TARGET} STATIC ${AML_MP_TESTSUPPORTER_SRC})

TARGET_LINK_LIBRARIES(${TARGET} PUBLIC aml_mp_sdk)

