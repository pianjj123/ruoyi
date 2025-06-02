迁移Darwin Streaming Server到 Visual Studio 2022
every thing changed: //_andy_: 
 //_andy_  :   commented off
 #define _64BITARG_ "I64"  //_andy_: add new def at top
 
 Q:\prj_Darwin_VS6\src\
 
 %I64d
2025.01.27
  command line  error D8016: '/ZI' and '/Gy-' command-line options are incompatible
	  "/Gy" is in C/C++ ➔ Code Generation ➔ Enable Function-Level Linking
	  "/ZI" is in C/C++ ➔ General ➔ Debug Information Format 
		   fix by replace "/ZI" with /Zi

	6>O:\prj_Darwin_VS_fix\src\qtpasswd.tproj\QTSSPasswd.cpp(391,74): error C3688: invalid literal suffix '__TIME__'; literal operator or literal operator template 'operator ""__TIME__' not found
		 //_andy_: commented off
		 
	QTAtom_stco.cpp(145,25): error C3688: invalid literal suffix '_64BITARG_'; literal operator or literal operator template 'operator ""_64BITARG_' not found
		//_andy_: add new def at top

	QTSSReflectorModule.cpp(866,13): error C2664: 'int _snprintf(char *const ,const size_t,const char *const ,...)': cannot convert argument 3 from 'SInt64' to 'const char *const '
	
	
	replace all:  	%"_64BITARG_"d  with 	%I64d
					%"_64BITARG_"u	with	%I64u
					"%11"_64BITARG_"u"   with "%11I647"
					
					


admin of user name:  admin
password: admin




Skip these warnings:




all projects surpess warning:  C/C++  -->  Advanced  --> Disable Specific Warnings:
	4005;4101;4244;4267;4474;4477;4778;4805;4819;4996;
4005
4101
4244
4267
4474
4477
4778
4805
4819
4996
	
	
	
add in vcxproj:
	<PropertyGroup>
		<MSBuildWarningsAsMessages>MSB8012</MSBuildWarningsAsMessages>
	</PropertyGroup>
	
6>- qtpasswd, Configuration
7>- RegistrySystemPathEditor, Configuration
10>- MP3Broadcaster, Configuration
11>- QTSSSpamDefenseModule, Configuration
12>- QTSSRefMovieModule, Configuration
13>- QTSSRawFileModule, Configuration
	C/C++ --> Code Generation --> Enable Minimal Rebuile: 



Get the Original Source Code from:
	https://github.com/macosforge/dss