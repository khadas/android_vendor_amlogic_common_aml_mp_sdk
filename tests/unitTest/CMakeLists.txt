project(amlMpUnitTest)

SET(AML_MP_UNITTEST_SRC
    AmlMpPlayerTest.cpp
    TestUrlList.cpp
)

SET(TARGET amlMpUnitTest)

ADD_EXECUTABLE(${TARGET} ${AML_MP_UNITTEST_SRC})

TARGET_INCLUDE_DIRECTORIES(${TARGET} PRIVATE ../amlMpTestSupporter)

TARGET_LINK_LIBRARIES(${TARGET} PUBLIC
    amlMpTestSupporter
    gtest
)

INSTALL(
    TARGETS ${TARGET}
)
