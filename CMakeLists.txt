cmake_minimum_required(VERSION 3.20)
####
# The version number in previous line may be reduced if needed. I used
# the default in cmake tutorial (oct 2021)
####

project(Ensishell)
enable_testing()

#########
# Gestion des équipes et variante du sujet / Team management and addition
#########
# Vous devez editer la lignes SET suivante pour y mettre vos logins
# et le numéro de la variante que vous avez choisi
#########
set(VARIANTE_SUJET 10)
### FIN de la zone à éditer / END of the two line area to edit

if (VARIANTE_SUJET EQUAL -1)
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier le CMakeLists.txt pour y mettre le numéro de votre variante du sujet / Edit VARIANTE_SUJET in the CMakeList")
endif()

if (EXISTS ${CMAKE_SOURCE_DIR}/AUTHORS)
  message(STATUS "Les auteurs du TP incrits dans AUTHORS sont / AUTHORS are:")
  execute_process (COMMAND cat AUTHORS WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
else()
  message(FATAL_ERROR "** ERREUR / ERROR **: Vous devez mettre vos prénoms, noms et login dans un fichier nommé 'AUTHORS' à la racine du projet / Add an 'AUTHORS' file with all authors' name, firstname and login at the project root; Fichier manquant/missing file: "  ${CMAKE_SOURCE_DIR}/AUTHORS)
endif()

# Use the 2017-2018 C standard with GNU extensions ("99" instead
# should work too with this skeleton)
set(CMAKE_C_STANDARD 17)

#######
# Detect presence of minitest using gem
# install it if not available
# CentOS7 use ruby 2.0 (2013), minitest > 5.11.3 asks 2.2.
# try to install 5.11.3 version of minitest if standard version failed
#######
execute_process(COMMAND gem which minitest RESULT_VARIABLE GEM_MINITEST)
if (NOT ${GEM_MINITEST} EQUAL 0)
  execute_process(COMMAND gem install minitest RESULT_VARIABLE GEM_MINITEST_INSTALL)
  if (NOT ${GEM_MINITEST_INSTALL} EQUAL 0)
      execute_process(COMMAND gem install minitest --version 5.11.3 RESULT_VARIABLE GEM_MINITEST_INSTALL_OLD)
      if (NOT ${GEM_MINITEST_INSTALL_OLD} EQUAL 0)
	message(FATAL_ERROR "Ruby: gem: minitest unavailable and uninstallable")
      endif ()
    endif ()
endif ()

#######
# Detect Guile using guile-config
# thus use the default "guile"
# guile-config is available in guile-2.2-dev and guile-3.0-dev

execute_process(COMMAND guile-config compile
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE GUILE_CFLAGS)
execute_process(COMMAND guile-config link
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE GUILE_LDFLAGS)

# If libguile is apparently not available, disable SCHEME support
set(USE_GUILE 1)
if ("${GUILE_LDFLAGS}" STREQUAL "")
  message(WARNING "*** libguile seems unvailable, disabling SCHEME support ***")
  set(USE_GUILE 0)
endif()

####
# Detect if gnu readline header is present, otherwise use internal readline
####
find_path(READLINE_PATH "include/readline/readline.h")
if (NOT "${READLINE_PATH}" STREQUAL "")
  set(USE_GNU_READLINE 1)
  set(READLINE_LDFLAGS readline history)
else()
  set(USE_GNU_READLINE 0)
  set(READLINE_LDFLAGS)
endif()


# Debug build
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${GUILE_CFLAGS} -Wall -Werror -fanalyzer")

# Nom des auteurs comme un identifiant C (monoligne alpha)
execute_process(COMMAND cat AUTHORS WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_VARIABLE AUTHOR_LIST)
string(STRIP "${AUTHOR_LIST}" AUTHOR_LIST)
string(MAKE_C_IDENTIFIER "${AUTHOR_LIST}" AUTHOR_LIST)

#########
# Gestion des variantes
#########
configure_file (
  src/variante.h.in
  ${CMAKE_SOURCE_DIR}/src/variante.h
)
#########
# Fin de gestion des variantes
#########

##
# Si vous utilisez plusieurs fichiers, en plus de ensishell.c, pour votre
# shell il faut les ajouter ici
##
add_executable(ensishell src/readcmd.c src/ensishell.c)
target_link_libraries(ensishell ${READLINE_LDFLAGS} ${GUILE_LDFLAGS})

##
# Programme de test
##
add_test(UnitShellTests ${CMAKE_SOURCE_DIR}/tests/allShellTests.rb)

##
# Ajout d'une cible pour lancer les tests de manière verbeuse
##
add_custom_target(check ${CMAKE_SOURCE_DIR}/tests/allShellTests.rb)

##
# Construction de l'archive
##

set(CPACK_PACKAGE_VERSION_MAJOR "2")
set(CPACK_PACKAGE_VERSION_MINOR "${VARIANTE_SUJET}")
set(CPACK_PACKAGE_VERSION_PATCH "0-${AUTHOR_LIST}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "\\\\.o$"
  "^${PROJECT_SOURCE_DIR}/build/"
  "^${PROJECT_SOURCE_DIR}/.git/"
)
include(CPack)
