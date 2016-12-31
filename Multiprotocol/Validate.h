// Check selected board type
#if defined (STM32_BOARD) && defined (ORANGE_TX)
	#error You must comment the board type STM32_BOARD in _Config.h to compile ORANGE_TX
#endif
#if ! defined (ORANGE_TX) && ! defined (STM32_BOARD)
	//Atmega328p
	#if ! defined(ARDUINO_AVR_PRO) && ! defined(ARDUINO_AVR_MINI) && ! defined(ARDUINO_AVR_NANO)
		#error You must select one of these boards: "Multi 4-in-1", "Arduino Pro or Pro Mini" or "Arduino Mini"
	#endif
	#if F_CPU != 16000000L || ! defined(__AVR_ATmega328P__)
		#error You must select the processor type "ATmega328(5V, 16MHz)"
	#endif
#endif
#if defined (STM32_BOARD) && ! defined (ORANGE_TX)
	//STM32
	#ifndef ARDUINO_GENERIC_STM32F103C
		#error You must select the board type "Generic STM32F103C series"
	#endif
#endif

//Change/Force configuration if OrangeTX
#ifdef ORANGE_TX
	#undef ENABLE_PPM			// Disable PPM for OrangeTX module
	#undef A7105_INSTALLED		// Disable A7105 for OrangeTX module
	#undef CC2500_INSTALLED		// Disable CC2500 for OrangeTX module
	#undef NRF24L01_INSTALLED	// Disable NRF for OrangeTX module
	#define TELEMETRY			// Enable telemetry
	#define INVERT_TELEMETRY	// Enable invert telemetry
	#define DSM_TELEMETRY		// Enable DSM telemetry
#endif

//Make sure protocols are selected correctly
#ifndef A7105_INSTALLED
	#undef FLYSKY_A7105_INO
	#undef HUBSAN_A7105_INO
	#undef AFHDS2A_A7105_INO
#endif
#ifndef CYRF6936_INSTALLED
	#undef	DEVO_CYRF6936_INO
	#undef	DSM_CYRF6936_INO
	#undef	J6PRO_CYRF6936_INO
#endif
#ifndef CC2500_INSTALLED
	#undef	FRSKYD_CC2500_INO
	#undef	FRSKYV_CC2500_INO
	#undef	FRSKYX_CC2500_INO
	#undef	SFHSS_CC2500_INO
#endif
#ifndef NRF24L01_INSTALLED
	#undef	BAYANG_NRF24L01_INO
	#undef	CG023_NRF24L01_INO
	#undef	CX10_NRF24L01_INO
	#undef	ESKY_NRF24L01_INO
	#undef	HISKY_NRF24L01_INO
	#undef	KN_NRF24L01_INO
	#undef	SLT_NRF24L01_INO
	#undef	SYMAX_NRF24L01_INO
	#undef	V2X2_NRF24L01_INO
	#undef	YD717_NRF24L01_INO
	#undef	MT99XX_NRF24L01_INO
	#undef	MJXQ_NRF24L01_INO
	#undef	SHENQI_NRF24L01_INO
	#undef	FY326_NRF24L01_INO
	#undef	FQ777_NRF24L01_INO
	#undef	ASSAN_NRF24L01_INO
	#undef	HONTAI_NRF24L01_INO
#endif

//Make sure telemetry is selected correctly
#ifndef TELEMETRY
	#undef INVERT_TELEMETRY
	#undef AFHDS2A_FW_TELEMETRY
	#undef AFHDS2A_HUB_TELEMETRY
	#undef BAYANG_HUB_TELEMETRY
	#undef HUBSAN_HUB_TELEMETRY
	#undef HUB_TELEMETRY
	#undef SPORT_TELEMETRY
	#undef DSM_TELEMETRY
	#undef MULTI_TELEMETRY
#else
	#if ! defined(BAYANG_NRF24L01_INO)
		#undef BAYANG_HUB_TELEMETRY
	#endif
	#if ! defined(HUBSAN_A7105_INO)
		#undef HUBSAN_HUB_TELEMETRY
	#endif
	#if ! defined(AFHDS2A_A7105_INO)
		#undef 	AFHDS2A_HUB_TELEMETRY
		#undef 	AFHDS2A_FW_TELEMETRY
	#endif
	#if ! defined(FRSKYD_CC2500_INO)
		#undef HUB_TELEMETRY
	#endif
	#if ! defined(FRSKYX_CC2500_INO)
		#undef SPORT_TELEMETRY
	#endif
	#if ! defined(DSM_CYRF6936_INO)
		#undef DSM_TELEMETRY
	#endif
	#if ! defined(DSM_TELEMETRY) && ! defined(SPORT_TELEMETRY) && ! defined(HUB_TELEMETRY) && ! defined(HUBSAN_HUB_TELEMETRY) && ! defined(BAYANG_HUB_TELEMETRY) && ! defined(AFHDS2A_HUB_TELEMETRY) && ! defined(AFHDS2A_FW_TELEMETRY)
		#undef TELEMETRY
		#undef INVERT_TELEMETRY
	#endif
#endif

//Make sure TX is defined correctly
#ifndef AILERON
	#error You must select a correct channel order.
#endif
#if ! defined(PPM_MAX_100) || ! defined(PPM_MIN_100) || ! defined(PPM_MAX_125) || ! defined(PPM_MIN_125)
	#error You must set correct TX end points.
#endif
