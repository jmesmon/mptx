cmake_minimum_required(VERSION 3.7)
project(multiprotocol)

set(CMAKE_USE_RELATIVE_PATHS TRUE)

include(CMakeForceCompiler)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
#SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

set(CMAKE_COMPILE_FLAGS "${CMAKE_COMPILE_FLAGS} -Wno-main -mmcu=atmega328p -Os -ggdb3")
add_definitions("-DF_CPU=16000000L")
add_definitions("-DARDUINO_AVR_PRO=1")
add_definitions("-DARDUINO_ARCH_AVR=1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_COMPILE_FLAGS}")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${CMAKE_COMPILE_FLAGS}")

set(ARDUINO_SDK "/usr/share/arduino")
set(ARDUINO_SRC pins_arduino.c wiring.c wiring_analog.c
	wiring_digital.c wiring_pulse.c wiring_serial.c
	wiring_shift.c WInterrupts.c
	HardwareSerial.cpp WMath.cpp)

function(make_absolute _base _out)
	set(_paths)
	foreach(_p ${ARGN})
		if (NOT IS_ABSOLUTE "${_p}")
			list(APPEND _paths "${_base}/${_p}")
		else()
			list(APPEND _paths "${_p}")
		endif()
	endforeach()

	set("${_out}" "${_paths}" PARENT_SCOPE)
endfunction()

function(add_arduino_exe _exe)
	set(_src)
	foreach(S ${ARGN})
		string(REGEX MATCH "[^\\.]+$" _ext "${S}")
		string(REGEX REPLACE "\\.[^\\.]*$" "" _f "${S}")
		if ("${_ext}" STREQUAL ino)
			set(_o "${CMAKE_CURRENT_BINARY_DIR}/${_f}.cc")
			set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${S}")
			add_custom_command(OUTPUT "${_o}"
				MAIN_DEPENDENCY "${SRC}"
				COMMAND ${CMAKE_COMMAND} -DSOURCE="${SRC}" -DOUTPUT="${_o}" -P "${CMAKE_SOURCE_DIR}/ino_to_cc.cmake"
			)
			list(APPEND _src "${_o}")
		else()
			list(APPEND _src "${S}")
		endif()
	endforeach()

	add_executable("${_exe}" ${_src} arduino_prefix.cc
		${ARDUINO_SDK}/libraries/Servo/src/avr/Servo.cpp)
	list(INSERT _src 0 arduino_prefix.cc)
	make_absolute("${CMAKE_CURRENT_SOURCE_DIR}" _src_abs ${_src})
	add_custom_command(OUTPUT combined.cc
		MAIN_DEPENDENCY arduino_prefix.cc
		DEPENDS ${_src}
		COMMAND ${CMAKE_COMMAND} -DCOMBINE_FILES="${_src_abs}" -DCOMBINE_OUTPUT="${CMAKE_CURRENT_BINARY_DIR}/combined.cc" -P "${CMAKE_SOURCE_DIR}/combine.cmake" 
	)
	#add_executable(${_exe}-combine.elf ${CMAKE_CURRENT_BINARY_DIR}/combined.cc)
	#target_include_directories(${_exe}-combine.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	message("Got src: ${_src}")
	set_target_properties("${_exe}" PROPERTIES SUFFIX ".elf")
	target_include_directories("${_exe}" PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR})
	target_include_directories("${_exe}" PRIVATE
		${ARDUINO_SDK}/hardware/arduino/avr/cores/arduino)
	target_include_directories("${_exe}" PRIVATE
		${ARDUINO_SDK}/hardware/arduino/avr/variants/eightanaloginputs)
	target_include_directories("${_exe}" PRIVATE
		${ARDUINO_SDK}/libraries/Servo/src/)
endfunction()

add_arduino_exe(multi
	A7105_SPI.ino AFHDS2A_a7105.ino ASSAN_nrf24l01.ino
	Arduino.ino Bayang_nrf24l01.ino CC2500_SPI.ino 	CG023_nrf24l01.ino
	CX10_nrf24l01.ino CYRF6936_SPI.ino Convert.ino DSM_cyrf6936.ino
	Devo_cyrf6936.ino ESky_nrf24l01.ino FQ777_nrf24l01.ino
	FY326_nrf24l01.ino FlySky_a7105.ino FrSkyD_cc2500.ino
	FrSkyV_cc2500.ino FrSkyX_cc2500.ino Hisky_nrf24l01.ino
	Hontai_nrf24l01.ino Hubsan_a7105.ino J6Pro_cyrf6936.ino
	KN_nrf24l01.ino MJXQ_nrf24l01.ino MT99xx_nrf24l01.ino
	Multiprotocol.ino NRF24l01_SPI.ino  SFHSS_cc2500.ino
	SHENQI_nrf24l01.ino SLT_nrf24l01.ino SPI.ino Symax_nrf24l01.ino
	Telemetry.ino V2X2_nrf24l01.ino YD717_nrf24l01.ino
)
