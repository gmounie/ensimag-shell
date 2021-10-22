cmake_minimum_required(VERSION 3.10)
####
# The version number in previous line may be reduced if needed. I used
# the default in cmake tutorial (oct 2021)
####

project(Ensishell)
enable_testing()

#########
# Gestion des équipes et variante du sujet / Team management and addition
#########
# Vous devez editer les deux lignes SET suivantes pour y mettre vos logins
# et le numéro de la variante que vous avez choisi
#########
set(VARIANTE_LOGINS login1 login2 login3)
set(VARIANTE_SUJET -1)
### FIN de la zone à éditer / END of the two line area to edit

list(SORT VARIANTE_LOGINS)
if (VARIANTE_LOGINS MATCHES "login[123]")
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre vos logins")
endif()
if (VARIANTE_SUJET EQUAL -1)
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre le numéro de votre variante du sujet")
endif()

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
# Use the last C standard with GNU extensions ("-std=gnu99" instead
# should work too with this skeleton)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${GUILE_CFLAGS} -Wall -Werror -std=gnu17")


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
add_test(UnitShellTests ../tests/allShellTests.rb)

##
# Ajout d'une cible pour lancer les tests de manière verbeuse
##
add_custom_target(check ../tests/allShellTests.rb)

##
# Construction de l'archive
##
string(REPLACE ";" "-" LOGINS_SANS_POINTVIRGULE "${VARIANTE_LOGINS}")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH ${LOGINS_SANS_POINTVIRGULE})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "\\\\.o$"
  "^${PROJECT_SOURCE_DIR}/build/"
  "^${PROJECT_SOURCE_DIR}/.git/"
)
include(CPack)
