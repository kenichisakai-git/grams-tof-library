# Tool to encapsulate some root dictionary building - making linkdefs, etc.
#
## Accepts the following:
#   * DICT_NAME         => will be the generated dictionary target.  If MODULE mode is used, no file will be made.
#   * DEST_TARGET       => allows you to call this AFTER another target named DEST_TARGET is defined.  Then this dictionary
#                       => will be added to the deps for DEST_TARGET
#   * CLASS_LIST        => a cmake list of all classes needing dictionaries (they will also get vector<>)
#   * CLASS_LIST_NO_IO  => a cmake list of all classes to be added to dictionary without IO
#   * CLASS_LIST_NO_VEC => a cmake list of all classes to be added to dictionary without vectors
#   * CLASS_LIST        => a cmake list of all classes needing dictionaries
#   * HEADER_LIST       => a cmake list of all the headers that these classes are in.
#                          this allows you to have multiple dictionary classes in some headers
#   * DEPS              => dependencies, like libs, that the dict will require
#
# https://sft.its.cern.ch/jira/browse/ROOT-8575
#
#
function(grams_root_dictionary_builder)
    cmake_parse_arguments(
            INT                                                              # prefix that gets added to variables INT='internal'
            ""                                                               # options
            "DICT_NAME;DEST_TARGET"                                          # ALL keywords that accept only 1 value
            "CLASS_LIST;HEADER_LIST;DEPS;CLASS_LIST_NO_VEC;CLASS_LIST_NO_IO" # ALL keywords that accept multiple values (see above)
            ${ARGN})

    set(INT_LINKDEF_NAME  ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${INT_DICT_NAME}_LinkDef.h)
    #set(INT_LINKDEF_MAKER ${GRAMS_Flight_Software_Common_SOURCE_DIR}/utils/LinkDefMakerMulti.py)
    set(INT_LINKDEF_MAKER ${CMAKE_SOURCE_DIR}/Common/utils/LinkDefMakerMulti.py)
    # this seems more robust than using GRAMS_Flight_Software_Common_SOURCE_DIR

    execute_process(COMMAND bash -c "if [[ ! -x ${INT_LINKDEF_MAKER} ]];then echo '=> Making LinkDefMaker executable' ; chmod u+x ${INT_LINKDEF_MAKER};fi")

    # build space-delimited version of classes.  These are only used by the LinkDefMaker.
    list(JOIN INT_CLASS_LIST        " " INT_CLASS_LIST_SP)        #normal classes
    list(JOIN INT_CLASS_LIST_NO_VEC " " INT_CLASS_LIST_NO_VEC_SP) #classes that should not have vector classes made
    list(JOIN INT_CLASS_LIST_NO_IO  " " INT_CLASS_LIST_NO_IO_SP)  #classes without IO!

#    message(STATUS "LinkDef Maker  : ${INT_LINKDEF_MAKER}")
#    message(STATUS "LinkDef Name   : ${INT_LINKDEF_NAME}")
#    message(STATUS "Class List     : ${INT_CLASS_LIST}")
#    message(STATUS "Class List Vec : ${INT_CLASS_LIST_NO_VEC}")
#    message(STATUS "Class List NIO : ${INT_CLASS_LIST_NO_IO}")
#    message(STATUS "Headers        : ${INT_HEADER_LIST}")
#    message(STATUS "Target Name    : ${INT_DICT_NAME}")
#    message(STATUS "Dest Target    : ${INT_DEST_TARGET}")
#    message(STATUS "Dependencies   : ${INT_DEPS}")

    message("${BYellow}-- Building ROOT Dictionary --${ColReset}")
    message("   => Output file location will be: ${Yellow}${INT_DICT_NAME}.cxx${ColReset}")
    if(INT_DEST_TARGET)
        message("   => Destination target:           ${BCyan}${INT_DEST_TARGET}${ColReset}")
    endif()

    # Pretty version - Report the dictionaries, splitting up list
    set(index 0)
    #list(SORT INT_CLASS_LIST COMPARE STRING)
    list(LENGTH INT_CLASS_LIST count)
    set(outline "   => ROOT Dictionaries needed for: ${BRed}")
    while(index LESS count)
        list(GET INT_CLASS_LIST ${index} item)
        string(APPEND outline "${item} ")    #<==== items added to output here
        math(EXPR ready "(${index}+1) % 4")
        if(ready EQUAL 0)
            message("${outline}${ColReset}")
            set(outline "                                    ${BRed}")
        endif(ready EQUAL 0)
        math(EXPR index "${index}+1")
    endwhile(index LESS count)
    message("${outline}${ColReset}")
    if(INT_CLASS_LIST_NO_IO)
        message("   => ROOT Dictionaries w/o IO:     ${BGreen}${INT_CLASS_LIST_NO_IO}${ColReset}")
    endif()
    if(INT_CLASS_LIST_NO_VEC)
        message("   => ROOT Dictionaries w/o vec<>   ${BGreen}${INT_CLASS_LIST_NO_VEC}${ColReset}")
    endif()
    if(INT_DEPS)
        message("   => Dependent targets:            ${Cyan}${INT_DEPS}${ColReset}")
    endif()
    message("")

    # Runs the custom LinkDefMaker - this is added as a dependency of ROOT_GENERATE_DICTIONARY
    # INT_CLASS_LIST_SP should be a SPACE-DELIMITED list of class names
    # using bash -c here because COMMAND is so picky... can never get it to behave
    add_custom_command(
            OUTPUT ${INT_LINKDEF_NAME}
            #COMMAND bash -c "${INT_LINKDEF_MAKER} ${INT_LINKDEF_NAME} ${INT_CLASS_LIST_SP}"
            COMMAND bash -c "${INT_LINKDEF_MAKER} ${INT_LINKDEF_NAME} --classes='${INT_CLASS_LIST_SP}' --classes_no_vec='${INT_CLASS_LIST_NO_VEC_SP}' --classes_no_io='${INT_CLASS_LIST_NO_IO_SP}'"
            COMMENT "Building LinkDef for ${INT_CLASS_LIST_SP}")

    # Actually do the generation - a single dictionary for all the appropriate classes
    if(INT_DEST_TARGET)
        root_generate_dictionary(${INT_DICT_NAME} ${INT_HEADER_LIST} ${DEP_HDR_DIRS} MODULE ${INT_DEST_TARGET} LINKDEF ${INT_LINKDEF_NAME} DEPENDENCIES ${INT_LINKDEF_NAME} ${INT_DEPS})
    else()
        root_generate_dictionary(${INT_DICT_NAME} ${INT_HEADER_LIST} ${DEP_HDR_DIRS} LINKDEF ${INT_LINKDEF_NAME} DEPENDENCIES ${INT_LINKDEF_NAME} ${INT_DEPS})
    endif()

endfunction(grams_root_dictionary_builder)
