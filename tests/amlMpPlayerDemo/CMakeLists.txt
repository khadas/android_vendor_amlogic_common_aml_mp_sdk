project(amlMpPlayerDemo)

SET(AML_MP_DEMO_SRC
    AmlMpPlayerDemo.cpp
)

SET(TARGET amlMpPlayerDemo)

ADD_EXECUTABLE(${TARGET} ${AML_MP_DEMO_SRC})

TARGET_INCLUDE_DIRECTORIES(${TARGET} PRIVATE ../amlMpTestSupporter)

TARGET_LINK_LIBRARIES(${TARGET} PUBLIC amlMpTestSupporter)

INSTALL(
    TARGETS ${TARGET}
)
