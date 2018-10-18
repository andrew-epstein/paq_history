int sttab[4096] = {
    -2047, -2047, -1984, -1856, -1770, -1728, -1685, -1648, -1616, -1584, -1552, -1525, -1504, -1482, -1461, -1440,
    -1418, -1402, -1390, -1378, -1367, -1355, -1344, -1332, -1320, -1309, -1297, -1285, -1276, -1269, -1262, -1255,
    -1248, -1240, -1233, -1226, -1219, -1212, -1205, -1198, -1191, -1184, -1176, -1169, -1162, -1155, -1149, -1145,
    -1140, -1136, -1131, -1126, -1122, -1117, -1113, -1108, -1104, -1099, -1094, -1090, -1085, -1081, -1076, -1072,
    -1067, -1062, -1058, -1053, -1049, -1044, -1040, -1035, -1030, -1026, -1022, -1019, -1017, -1014, -1011, -1009,
    -1006, -1003, -1000, -998,  -995,  -992,  -989,  -987,  -984,  -981,  -979,  -976,  -973,  -970,  -968,  -965,
    -962,  -960,  -957,  -954,  -951,  -949,  -946,  -943,  -940,  -938,  -935,  -932,  -930,  -927,  -924,  -921,
    -919,  -916,  -913,  -910,  -908,  -905,  -902,  -900,  -897,  -895,  -893,  -891,  -889,  -888,  -886,  -884,
    -883,  -881,  -879,  -877,  -876,  -874,  -872,  -870,  -869,  -867,  -865,  -864,  -862,  -860,  -858,  -857,
    -855,  -853,  -851,  -850,  -848,  -846,  -844,  -843,  -841,  -839,  -838,  -836,  -834,  -832,  -831,  -829,
    -827,  -825,  -824,  -822,  -820,  -819,  -817,  -815,  -813,  -812,  -810,  -808,  -806,  -805,  -803,  -801,
    -800,  -798,  -796,  -794,  -793,  -791,  -789,  -787,  -786,  -784,  -782,  -780,  -779,  -777,  -775,  -774,
    -772,  -770,  -768,  -767,  -766,  -765,  -764,  -763,  -761,  -760,  -759,  -758,  -757,  -756,  -755,  -754,
    -753,  -752,  -750,  -749,  -748,  -747,  -746,  -745,  -744,  -743,  -742,  -740,  -739,  -738,  -737,  -736,
    -735,  -734,  -733,  -732,  -731,  -729,  -728,  -727,  -726,  -725,  -724,  -723,  -722,  -721,  -720,  -718,
    -717,  -716,  -715,  -714,  -713,  -712,  -711,  -710,  -708,  -707,  -706,  -705,  -704,  -703,  -702,  -701,
    -700,  -699,  -697,  -696,  -695,  -694,  -693,  -692,  -691,  -690,  -689,  -688,  -686,  -685,  -684,  -683,
    -682,  -681,  -680,  -679,  -678,  -676,  -675,  -674,  -673,  -672,  -671,  -670,  -669,  -668,  -667,  -665,
    -664,  -663,  -662,  -661,  -660,  -659,  -658,  -657,  -656,  -654,  -653,  -652,  -651,  -650,  -649,  -648,
    -647,  -646,  -644,  -643,  -642,  -641,  -640,  -639,  -638,  -638,  -637,  -636,  -636,  -635,  -634,  -633,
    -633,  -632,  -631,  -631,  -630,  -629,  -628,  -628,  -627,  -626,  -625,  -625,  -624,  -623,  -623,  -622,
    -621,  -620,  -620,  -619,  -618,  -618,  -617,  -616,  -615,  -615,  -614,  -613,  -613,  -612,  -611,  -610,
    -610,  -609,  -608,  -608,  -607,  -606,  -605,  -605,  -604,  -603,  -602,  -602,  -601,  -600,  -600,  -599,
    -598,  -597,  -597,  -596,  -595,  -595,  -594,  -593,  -592,  -592,  -591,  -590,  -590,  -589,  -588,  -587,
    -587,  -586,  -585,  -584,  -584,  -583,  -582,  -582,  -581,  -580,  -579,  -579,  -578,  -577,  -577,  -576,
    -575,  -574,  -574,  -573,  -572,  -572,  -571,  -570,  -569,  -569,  -568,  -567,  -567,  -566,  -565,  -564,
    -564,  -563,  -562,  -561,  -561,  -560,  -559,  -559,  -558,  -557,  -556,  -556,  -555,  -554,  -554,  -553,
    -552,  -551,  -551,  -550,  -549,  -549,  -548,  -547,  -546,  -546,  -545,  -544,  -544,  -543,  -542,  -541,
    -541,  -540,  -539,  -538,  -538,  -537,  -536,  -536,  -535,  -534,  -533,  -533,  -532,  -531,  -531,  -530,
    -529,  -528,  -528,  -527,  -526,  -526,  -525,  -524,  -523,  -523,  -522,  -521,  -520,  -520,  -519,  -518,
    -518,  -517,  -516,  -515,  -515,  -514,  -513,  -513,  -512,  -511,  -511,  -510,  -510,  -509,  -509,  -508,
    -508,  -507,  -507,  -506,  -506,  -505,  -505,  -504,  -504,  -503,  -503,  -502,  -502,  -501,  -501,  -500,
    -500,  -499,  -499,  -498,  -498,  -497,  -497,  -496,  -496,  -495,  -495,  -494,  -494,  -493,  -493,  -492,
    -492,  -491,  -491,  -490,  -490,  -490,  -489,  -489,  -488,  -488,  -487,  -487,  -486,  -486,  -485,  -485,
    -484,  -484,  -483,  -483,  -482,  -482,  -481,  -481,  -480,  -480,  -479,  -479,  -478,  -478,  -477,  -477,
    -476,  -476,  -475,  -475,  -474,  -474,  -473,  -473,  -472,  -472,  -471,  -471,  -470,  -470,  -469,  -469,
    -468,  -468,  -467,  -467,  -466,  -466,  -465,  -465,  -464,  -464,  -463,  -463,  -462,  -462,  -461,  -461,
    -460,  -460,  -459,  -459,  -458,  -458,  -457,  -457,  -456,  -456,  -455,  -455,  -454,  -454,  -453,  -453,
    -452,  -452,  -451,  -451,  -450,  -450,  -449,  -449,  -448,  -448,  -448,  -447,  -447,  -446,  -446,  -445,
    -445,  -444,  -444,  -443,  -443,  -442,  -442,  -441,  -441,  -440,  -440,  -439,  -439,  -438,  -438,  -437,
    -437,  -436,  -436,  -435,  -435,  -434,  -434,  -433,  -433,  -432,  -432,  -431,  -431,  -430,  -430,  -429,
    -429,  -428,  -428,  -427,  -427,  -426,  -426,  -425,  -425,  -424,  -424,  -423,  -423,  -422,  -422,  -421,
    -421,  -420,  -420,  -419,  -419,  -418,  -418,  -417,  -417,  -416,  -416,  -415,  -415,  -414,  -414,  -413,
    -413,  -412,  -412,  -411,  -411,  -410,  -410,  -409,  -409,  -408,  -408,  -407,  -407,  -406,  -406,  -405,
    -405,  -405,  -404,  -404,  -403,  -403,  -402,  -402,  -401,  -401,  -400,  -400,  -399,  -399,  -398,  -398,
    -397,  -397,  -396,  -396,  -395,  -395,  -394,  -394,  -393,  -393,  -392,  -392,  -391,  -391,  -390,  -390,
    -389,  -389,  -388,  -388,  -387,  -387,  -386,  -386,  -385,  -385,  -384,  -384,  -383,  -383,  -383,  -382,
    -382,  -382,  -381,  -381,  -380,  -380,  -380,  -379,  -379,  -379,  -378,  -378,  -378,  -377,  -377,  -376,
    -376,  -376,  -375,  -375,  -375,  -374,  -374,  -374,  -373,  -373,  -372,  -372,  -372,  -371,  -371,  -371,
    -370,  -370,  -370,  -369,  -369,  -368,  -368,  -368,  -367,  -367,  -367,  -366,  -366,  -366,  -365,  -365,
    -365,  -364,  -364,  -363,  -363,  -363,  -362,  -362,  -362,  -361,  -361,  -361,  -360,  -360,  -359,  -359,
    -359,  -358,  -358,  -358,  -357,  -357,  -357,  -356,  -356,  -355,  -355,  -355,  -354,  -354,  -354,  -353,
    -353,  -353,  -352,  -352,  -352,  -351,  -351,  -350,  -350,  -350,  -349,  -349,  -349,  -348,  -348,  -348,
    -347,  -347,  -346,  -346,  -346,  -345,  -345,  -345,  -344,  -344,  -344,  -343,  -343,  -342,  -342,  -342,
    -341,  -341,  -341,  -340,  -340,  -340,  -339,  -339,  -338,  -338,  -338,  -337,  -337,  -337,  -336,  -336,
    -336,  -335,  -335,  -335,  -334,  -334,  -333,  -333,  -333,  -332,  -332,  -332,  -331,  -331,  -331,  -330,
    -330,  -329,  -329,  -329,  -328,  -328,  -328,  -327,  -327,  -327,  -326,  -326,  -325,  -325,  -325,  -324,
    -324,  -324,  -323,  -323,  -323,  -322,  -322,  -321,  -321,  -321,  -320,  -320,  -320,  -319,  -319,  -319,
    -318,  -318,  -318,  -317,  -317,  -316,  -316,  -316,  -315,  -315,  -315,  -314,  -314,  -314,  -313,  -313,
    -312,  -312,  -312,  -311,  -311,  -311,  -310,  -310,  -310,  -309,  -309,  -308,  -308,  -308,  -307,  -307,
    -307,  -306,  -306,  -306,  -305,  -305,  -304,  -304,  -304,  -303,  -303,  -303,  -302,  -302,  -302,  -301,
    -301,  -301,  -300,  -300,  -299,  -299,  -299,  -298,  -298,  -298,  -297,  -297,  -297,  -296,  -296,  -295,
    -295,  -295,  -294,  -294,  -294,  -293,  -293,  -293,  -292,  -292,  -291,  -291,  -291,  -290,  -290,  -290,
    -289,  -289,  -289,  -288,  -288,  -288,  -287,  -287,  -286,  -286,  -286,  -285,  -285,  -285,  -284,  -284,
    -284,  -283,  -283,  -282,  -282,  -282,  -281,  -281,  -281,  -280,  -280,  -280,  -279,  -279,  -278,  -278,
    -278,  -277,  -277,  -277,  -276,  -276,  -276,  -275,  -275,  -274,  -274,  -274,  -273,  -273,  -273,  -272,
    -272,  -272,  -271,  -271,  -271,  -270,  -270,  -269,  -269,  -269,  -268,  -268,  -268,  -267,  -267,  -267,
    -266,  -266,  -265,  -265,  -265,  -264,  -264,  -264,  -263,  -263,  -263,  -262,  -262,  -261,  -261,  -261,
    -260,  -260,  -260,  -259,  -259,  -259,  -258,  -258,  -257,  -257,  -257,  -256,  -256,  -256,  -255,  -255,
    -255,  -254,  -254,  -254,  -254,  -253,  -253,  -253,  -252,  -252,  -252,  -252,  -251,  -251,  -251,  -250,
    -250,  -250,  -250,  -249,  -249,  -249,  -248,  -248,  -248,  -248,  -247,  -247,  -247,  -246,  -246,  -246,
    -246,  -245,  -245,  -245,  -244,  -244,  -244,  -244,  -243,  -243,  -243,  -242,  -242,  -242,  -242,  -241,
    -241,  -241,  -240,  -240,  -240,  -240,  -239,  -239,  -239,  -238,  -238,  -238,  -238,  -237,  -237,  -237,
    -236,  -236,  -236,  -236,  -235,  -235,  -235,  -234,  -234,  -234,  -233,  -233,  -233,  -233,  -232,  -232,
    -232,  -231,  -231,  -231,  -231,  -230,  -230,  -230,  -229,  -229,  -229,  -229,  -228,  -228,  -228,  -227,
    -227,  -227,  -227,  -226,  -226,  -226,  -225,  -225,  -225,  -225,  -224,  -224,  -224,  -223,  -223,  -223,
    -223,  -222,  -222,  -222,  -221,  -221,  -221,  -221,  -220,  -220,  -220,  -219,  -219,  -219,  -219,  -218,
    -218,  -218,  -217,  -217,  -217,  -217,  -216,  -216,  -216,  -215,  -215,  -215,  -215,  -214,  -214,  -214,
    -213,  -213,  -213,  -212,  -212,  -212,  -212,  -211,  -211,  -211,  -210,  -210,  -210,  -210,  -209,  -209,
    -209,  -208,  -208,  -208,  -208,  -207,  -207,  -207,  -206,  -206,  -206,  -206,  -205,  -205,  -205,  -204,
    -204,  -204,  -204,  -203,  -203,  -203,  -202,  -202,  -202,  -202,  -201,  -201,  -201,  -200,  -200,  -200,
    -200,  -199,  -199,  -199,  -198,  -198,  -198,  -198,  -197,  -197,  -197,  -196,  -196,  -196,  -196,  -195,
    -195,  -195,  -194,  -194,  -194,  -194,  -193,  -193,  -193,  -192,  -192,  -192,  -192,  -191,  -191,  -191,
    -190,  -190,  -190,  -189,  -189,  -189,  -189,  -188,  -188,  -188,  -187,  -187,  -187,  -187,  -186,  -186,
    -186,  -185,  -185,  -185,  -185,  -184,  -184,  -184,  -183,  -183,  -183,  -183,  -182,  -182,  -182,  -181,
    -181,  -181,  -181,  -180,  -180,  -180,  -179,  -179,  -179,  -179,  -178,  -178,  -178,  -177,  -177,  -177,
    -177,  -176,  -176,  -176,  -175,  -175,  -175,  -175,  -174,  -174,  -174,  -173,  -173,  -173,  -173,  -172,
    -172,  -172,  -171,  -171,  -171,  -171,  -170,  -170,  -170,  -169,  -169,  -169,  -168,  -168,  -168,  -168,
    -167,  -167,  -167,  -166,  -166,  -166,  -166,  -165,  -165,  -165,  -164,  -164,  -164,  -164,  -163,  -163,
    -163,  -162,  -162,  -162,  -162,  -161,  -161,  -161,  -160,  -160,  -160,  -160,  -159,  -159,  -159,  -158,
    -158,  -158,  -158,  -157,  -157,  -157,  -156,  -156,  -156,  -156,  -155,  -155,  -155,  -154,  -154,  -154,
    -154,  -153,  -153,  -153,  -152,  -152,  -152,  -152,  -151,  -151,  -151,  -150,  -150,  -150,  -150,  -149,
    -149,  -149,  -148,  -148,  -148,  -147,  -147,  -147,  -147,  -146,  -146,  -146,  -145,  -145,  -145,  -145,
    -144,  -144,  -144,  -143,  -143,  -143,  -143,  -142,  -142,  -142,  -141,  -141,  -141,  -141,  -140,  -140,
    -140,  -139,  -139,  -139,  -139,  -138,  -138,  -138,  -137,  -137,  -137,  -137,  -136,  -136,  -136,  -135,
    -135,  -135,  -135,  -134,  -134,  -134,  -133,  -133,  -133,  -133,  -132,  -132,  -132,  -131,  -131,  -131,
    -131,  -130,  -130,  -130,  -129,  -129,  -129,  -129,  -128,  -128,  -128,  -127,  -127,  -127,  -127,  -126,
    -126,  -126,  -126,  -125,  -125,  -125,  -125,  -124,  -124,  -124,  -124,  -123,  -123,  -123,  -123,  -122,
    -122,  -122,  -121,  -121,  -121,  -121,  -120,  -120,  -120,  -120,  -119,  -119,  -119,  -119,  -118,  -118,
    -118,  -118,  -117,  -117,  -117,  -117,  -116,  -116,  -116,  -116,  -115,  -115,  -115,  -115,  -114,  -114,
    -114,  -114,  -113,  -113,  -113,  -113,  -112,  -112,  -112,  -112,  -111,  -111,  -111,  -111,  -110,  -110,
    -110,  -109,  -109,  -109,  -109,  -108,  -108,  -108,  -108,  -107,  -107,  -107,  -107,  -106,  -106,  -106,
    -106,  -105,  -105,  -105,  -105,  -104,  -104,  -104,  -104,  -103,  -103,  -103,  -103,  -102,  -102,  -102,
    -102,  -101,  -101,  -101,  -101,  -100,  -100,  -100,  -100,  -99,   -99,   -99,   -99,   -98,   -98,   -98,
    -97,   -97,   -97,   -97,   -96,   -96,   -96,   -96,   -95,   -95,   -95,   -95,   -94,   -94,   -94,   -94,
    -93,   -93,   -93,   -93,   -92,   -92,   -92,   -92,   -91,   -91,   -91,   -91,   -90,   -90,   -90,   -90,
    -89,   -89,   -89,   -89,   -88,   -88,   -88,   -88,   -87,   -87,   -87,   -86,   -86,   -86,   -86,   -85,
    -85,   -85,   -85,   -84,   -84,   -84,   -84,   -83,   -83,   -83,   -83,   -82,   -82,   -82,   -82,   -81,
    -81,   -81,   -81,   -80,   -80,   -80,   -80,   -79,   -79,   -79,   -79,   -78,   -78,   -78,   -78,   -77,
    -77,   -77,   -77,   -76,   -76,   -76,   -76,   -75,   -75,   -75,   -74,   -74,   -74,   -74,   -73,   -73,
    -73,   -73,   -72,   -72,   -72,   -72,   -71,   -71,   -71,   -71,   -70,   -70,   -70,   -70,   -69,   -69,
    -69,   -69,   -68,   -68,   -68,   -68,   -67,   -67,   -67,   -67,   -66,   -66,   -66,   -66,   -65,   -65,
    -65,   -65,   -64,   -64,   -64,   -64,   -63,   -63,   -63,   -62,   -62,   -62,   -62,   -61,   -61,   -61,
    -61,   -60,   -60,   -60,   -60,   -59,   -59,   -59,   -59,   -58,   -58,   -58,   -58,   -57,   -57,   -57,
    -57,   -56,   -56,   -56,   -56,   -55,   -55,   -55,   -55,   -54,   -54,   -54,   -54,   -53,   -53,   -53,
    -53,   -52,   -52,   -52,   -51,   -51,   -51,   -51,   -50,   -50,   -50,   -50,   -49,   -49,   -49,   -49,
    -48,   -48,   -48,   -48,   -47,   -47,   -47,   -47,   -46,   -46,   -46,   -46,   -45,   -45,   -45,   -45,
    -44,   -44,   -44,   -44,   -43,   -43,   -43,   -43,   -42,   -42,   -42,   -42,   -41,   -41,   -41,   -41,
    -40,   -40,   -40,   -39,   -39,   -39,   -39,   -38,   -38,   -38,   -38,   -37,   -37,   -37,   -37,   -36,
    -36,   -36,   -36,   -35,   -35,   -35,   -35,   -34,   -34,   -34,   -34,   -33,   -33,   -33,   -33,   -32,
    -32,   -32,   -32,   -31,   -31,   -31,   -31,   -30,   -30,   -30,   -30,   -29,   -29,   -29,   -28,   -28,
    -28,   -28,   -27,   -27,   -27,   -27,   -26,   -26,   -26,   -26,   -25,   -25,   -25,   -25,   -24,   -24,
    -24,   -24,   -23,   -23,   -23,   -23,   -22,   -22,   -22,   -22,   -21,   -21,   -21,   -21,   -20,   -20,
    -20,   -20,   -19,   -19,   -19,   -19,   -18,   -18,   -18,   -18,   -17,   -17,   -17,   -16,   -16,   -16,
    -16,   -15,   -15,   -15,   -15,   -14,   -14,   -14,   -14,   -13,   -13,   -13,   -13,   -12,   -12,   -12,
    -12,   -11,   -11,   -11,   -11,   -10,   -10,   -10,   -10,   -9,    -9,    -9,    -9,    -8,    -8,    -8,
    -8,    -7,    -7,    -7,    -7,    -6,    -6,    -6,    -6,    -5,    -5,    -5,    -4,    -4,    -4,    -4,
    -3,    -3,    -3,    -3,    -2,    -2,    -2,    -2,    -1,    -1,    -1,    -1,    0,     0,     0,     0,
    1,     1,     1,     1,     2,     2,     2,     2,     3,     3,     3,     3,     4,     4,     4,     4,
    5,     5,     5,     5,     6,     6,     6,     6,     7,     7,     7,     8,     8,     8,     8,     9,
    9,     9,     9,     10,    10,    10,    10,    11,    11,    11,    11,    12,    12,    12,    12,    13,
    13,    13,    13,    14,    14,    14,    14,    15,    15,    15,    15,    16,    16,    16,    16,    17,
    17,    17,    17,    18,    18,    18,    18,    19,    19,    19,    19,    20,    20,    20,    21,    21,
    21,    21,    22,    22,    22,    22,    23,    23,    23,    23,    24,    24,    24,    24,    25,    25,
    25,    25,    26,    26,    26,    26,    27,    27,    27,    27,    28,    28,    28,    28,    29,    29,
    29,    29,    30,    30,    30,    30,    31,    31,    31,    31,    32,    32,    32,    32,    33,    33,
    33,    34,    34,    34,    34,    35,    35,    35,    35,    36,    36,    36,    36,    37,    37,    37,
    37,    38,    38,    38,    38,    39,    39,    39,    39,    40,    40,    40,    40,    41,    41,    41,
    41,    42,    42,    42,    42,    43,    43,    43,    43,    44,    44,    44,    44,    45,    45,    45,
    46,    46,    46,    46,    47,    47,    47,    47,    48,    48,    48,    48,    49,    49,    49,    49,
    50,    50,    50,    50,    51,    51,    51,    51,    52,    52,    52,    52,    53,    53,    53,    53,
    54,    54,    54,    54,    55,    55,    55,    55,    56,    56,    56,    56,    57,    57,    57,    57,
    58,    58,    58,    59,    59,    59,    59,    60,    60,    60,    60,    61,    61,    61,    61,    62,
    62,    62,    62,    63,    63,    63,    63,    64,    64,    64,    64,    65,    65,    65,    65,    66,
    66,    66,    66,    67,    67,    67,    67,    68,    68,    68,    68,    69,    69,    69,    69,    70,
    70,    70,    70,    71,    71,    71,    72,    72,    72,    72,    73,    73,    73,    73,    74,    74,
    74,    74,    75,    75,    75,    75,    76,    76,    76,    76,    77,    77,    77,    77,    78,    78,
    78,    78,    79,    79,    79,    79,    80,    80,    80,    80,    81,    81,    81,    81,    82,    82,
    82,    82,    83,    83,    83,    83,    84,    84,    84,    85,    85,    85,    85,    86,    86,    86,
    86,    87,    87,    87,    87,    88,    88,    88,    88,    89,    89,    89,    89,    90,    90,    90,
    90,    91,    91,    91,    91,    92,    92,    92,    92,    93,    93,    93,    93,    94,    94,    94,
    94,    95,    95,    95,    95,    96,    96,    96,    96,    97,    97,    97,    98,    98,    98,    98,
    99,    99,    99,    99,    100,   100,   100,   100,   101,   101,   101,   101,   102,   102,   102,   102,
    103,   103,   103,   103,   104,   104,   104,   104,   105,   105,   105,   105,   106,   106,   106,   106,
    107,   107,   107,   107,   108,   108,   108,   108,   109,   109,   109,   110,   110,   110,   110,   111,
    111,   111,   111,   112,   112,   112,   112,   113,   113,   113,   113,   114,   114,   114,   114,   115,
    115,   115,   115,   116,   116,   116,   116,   117,   117,   117,   117,   118,   118,   118,   118,   119,
    119,   119,   119,   120,   120,   120,   120,   121,   121,   121,   121,   122,   122,   122,   123,   123,
    123,   123,   124,   124,   124,   124,   125,   125,   125,   125,   126,   126,   126,   126,   127,   127,
    127,   127,   128,   128,   128,   128,   129,   129,   129,   130,   130,   130,   130,   131,   131,   131,
    132,   132,   132,   132,   133,   133,   133,   134,   134,   134,   134,   135,   135,   135,   136,   136,
    136,   136,   137,   137,   137,   138,   138,   138,   138,   139,   139,   139,   140,   140,   140,   140,
    141,   141,   141,   142,   142,   142,   142,   143,   143,   143,   144,   144,   144,   144,   145,   145,
    145,   146,   146,   146,   146,   147,   147,   147,   148,   148,   148,   148,   149,   149,   149,   150,
    150,   150,   151,   151,   151,   151,   152,   152,   152,   153,   153,   153,   153,   154,   154,   154,
    155,   155,   155,   155,   156,   156,   156,   157,   157,   157,   157,   158,   158,   158,   159,   159,
    159,   159,   160,   160,   160,   161,   161,   161,   161,   162,   162,   162,   163,   163,   163,   163,
    164,   164,   164,   165,   165,   165,   165,   166,   166,   166,   167,   167,   167,   167,   168,   168,
    168,   169,   169,   169,   169,   170,   170,   170,   171,   171,   171,   172,   172,   172,   172,   173,
    173,   173,   174,   174,   174,   174,   175,   175,   175,   176,   176,   176,   176,   177,   177,   177,
    178,   178,   178,   178,   179,   179,   179,   180,   180,   180,   180,   181,   181,   181,   182,   182,
    182,   182,   183,   183,   183,   184,   184,   184,   184,   185,   185,   185,   186,   186,   186,   186,
    187,   187,   187,   188,   188,   188,   188,   189,   189,   189,   190,   190,   190,   190,   191,   191,
    191,   192,   192,   192,   192,   193,   193,   193,   194,   194,   194,   195,   195,   195,   195,   196,
    196,   196,   197,   197,   197,   197,   198,   198,   198,   199,   199,   199,   199,   200,   200,   200,
    201,   201,   201,   201,   202,   202,   202,   203,   203,   203,   203,   204,   204,   204,   205,   205,
    205,   205,   206,   206,   206,   207,   207,   207,   207,   208,   208,   208,   209,   209,   209,   209,
    210,   210,   210,   211,   211,   211,   211,   212,   212,   212,   213,   213,   213,   213,   214,   214,
    214,   215,   215,   215,   216,   216,   216,   216,   217,   217,   217,   218,   218,   218,   218,   219,
    219,   219,   220,   220,   220,   220,   221,   221,   221,   222,   222,   222,   222,   223,   223,   223,
    224,   224,   224,   224,   225,   225,   225,   226,   226,   226,   226,   227,   227,   227,   228,   228,
    228,   228,   229,   229,   229,   230,   230,   230,   230,   231,   231,   231,   232,   232,   232,   232,
    233,   233,   233,   234,   234,   234,   234,   235,   235,   235,   236,   236,   236,   237,   237,   237,
    237,   238,   238,   238,   239,   239,   239,   239,   240,   240,   240,   241,   241,   241,   241,   242,
    242,   242,   243,   243,   243,   243,   244,   244,   244,   245,   245,   245,   245,   246,   246,   246,
    247,   247,   247,   247,   248,   248,   248,   249,   249,   249,   249,   250,   250,   250,   251,   251,
    251,   251,   252,   252,   252,   253,   253,   253,   253,   254,   254,   254,   255,   255,   255,   255,
    256,   256,   256,   257,   257,   257,   258,   258,   258,   259,   259,   260,   260,   260,   261,   261,
    261,   262,   262,   262,   263,   263,   264,   264,   264,   265,   265,   265,   266,   266,   266,   267,
    267,   268,   268,   268,   269,   269,   269,   270,   270,   270,   271,   271,   272,   272,   272,   273,
    273,   273,   274,   274,   274,   275,   275,   275,   276,   276,   277,   277,   277,   278,   278,   278,
    279,   279,   279,   280,   280,   281,   281,   281,   282,   282,   282,   283,   283,   283,   284,   284,
    285,   285,   285,   286,   286,   286,   287,   287,   287,   288,   288,   288,   289,   289,   290,   290,
    290,   291,   291,   291,   292,   292,   292,   293,   293,   294,   294,   294,   295,   295,   295,   296,
    296,   296,   297,   297,   298,   298,   298,   299,   299,   299,   300,   300,   300,   301,   301,   302,
    302,   302,   303,   303,   303,   304,   304,   304,   305,   305,   305,   306,   306,   307,   307,   307,
    308,   308,   308,   309,   309,   309,   310,   310,   311,   311,   311,   312,   312,   312,   313,   313,
    313,   314,   314,   315,   315,   315,   316,   316,   316,   317,   317,   317,   318,   318,   319,   319,
    319,   320,   320,   320,   321,   321,   321,   322,   322,   322,   323,   323,   324,   324,   324,   325,
    325,   325,   326,   326,   326,   327,   327,   328,   328,   328,   329,   329,   329,   330,   330,   330,
    331,   331,   332,   332,   332,   333,   333,   333,   334,   334,   334,   335,   335,   336,   336,   336,
    337,   337,   337,   338,   338,   338,   339,   339,   339,   340,   340,   341,   341,   341,   342,   342,
    342,   343,   343,   343,   344,   344,   345,   345,   345,   346,   346,   346,   347,   347,   347,   348,
    348,   349,   349,   349,   350,   350,   350,   351,   351,   351,   352,   352,   352,   353,   353,   354,
    354,   354,   355,   355,   355,   356,   356,   356,   357,   357,   358,   358,   358,   359,   359,   359,
    360,   360,   360,   361,   361,   362,   362,   362,   363,   363,   363,   364,   364,   364,   365,   365,
    366,   366,   366,   367,   367,   367,   368,   368,   368,   369,   369,   369,   370,   370,   371,   371,
    371,   372,   372,   372,   373,   373,   373,   374,   374,   375,   375,   375,   376,   376,   376,   377,
    377,   377,   378,   378,   379,   379,   379,   380,   380,   380,   381,   381,   381,   382,   382,   383,
    383,   383,   384,   384,   384,   385,   385,   386,   386,   387,   387,   388,   388,   389,   389,   390,
    390,   391,   391,   392,   392,   393,   393,   394,   394,   395,   395,   396,   396,   397,   397,   398,
    398,   399,   399,   400,   400,   401,   401,   402,   402,   403,   403,   404,   404,   405,   405,   406,
    406,   406,   407,   407,   408,   408,   409,   409,   410,   410,   411,   411,   412,   412,   413,   413,
    414,   414,   415,   415,   416,   416,   417,   417,   418,   418,   419,   419,   420,   420,   421,   421,
    422,   422,   423,   423,   424,   424,   425,   425,   426,   426,   427,   427,   428,   428,   429,   429,
    430,   430,   431,   431,   432,   432,   433,   433,   434,   434,   435,   435,   436,   436,   437,   437,
    438,   438,   439,   439,   440,   440,   441,   441,   442,   442,   443,   443,   444,   444,   445,   445,
    446,   446,   447,   447,   448,   448,   448,   449,   449,   450,   450,   451,   451,   452,   452,   453,
    453,   454,   454,   455,   455,   456,   456,   457,   457,   458,   458,   459,   459,   460,   460,   461,
    461,   462,   462,   463,   463,   464,   464,   465,   465,   466,   466,   467,   467,   468,   468,   469,
    469,   470,   470,   471,   471,   472,   472,   473,   473,   474,   474,   475,   475,   476,   476,   477,
    477,   478,   478,   479,   479,   480,   480,   481,   481,   482,   482,   483,   483,   484,   484,   485,
    485,   486,   486,   487,   487,   488,   488,   489,   489,   490,   490,   491,   491,   491,   492,   492,
    493,   493,   494,   494,   495,   495,   496,   496,   497,   497,   498,   498,   499,   499,   500,   500,
    501,   501,   502,   502,   503,   503,   504,   504,   505,   505,   506,   506,   507,   507,   508,   508,
    509,   509,   510,   510,   511,   511,   512,   512,   513,   514,   514,   515,   516,   516,   517,   518,
    519,   519,   520,   521,   521,   522,   523,   524,   524,   525,   526,   527,   527,   528,   529,   529,
    530,   531,   532,   532,   533,   534,   534,   535,   536,   537,   537,   538,   539,   539,   540,   541,
    542,   542,   543,   544,   544,   545,   546,   547,   547,   548,   549,   550,   550,   551,   552,   552,
    553,   554,   555,   555,   556,   557,   557,   558,   559,   560,   560,   561,   562,   562,   563,   564,
    565,   565,   566,   567,   568,   568,   569,   570,   570,   571,   572,   573,   573,   574,   575,   575,
    576,   577,   578,   578,   579,   580,   580,   581,   582,   583,   583,   584,   585,   585,   586,   587,
    588,   588,   589,   590,   591,   591,   592,   593,   593,   594,   595,   596,   596,   597,   598,   598,
    599,   600,   601,   601,   602,   603,   603,   604,   605,   606,   606,   607,   608,   608,   609,   610,
    611,   611,   612,   613,   614,   614,   615,   616,   616,   617,   618,   619,   619,   620,   621,   621,
    622,   623,   624,   624,   625,   626,   626,   627,   628,   629,   629,   630,   631,   632,   632,   633,
    634,   634,   635,   636,   637,   637,   638,   639,   639,   640,   641,   642,   643,   644,   645,   647,
    648,   649,   650,   651,   652,   653,   654,   655,   656,   658,   659,   660,   661,   662,   663,   664,
    665,   666,   668,   669,   670,   671,   672,   673,   674,   675,   676,   677,   679,   680,   681,   682,
    683,   684,   685,   686,   687,   688,   690,   691,   692,   693,   694,   695,   696,   697,   698,   700,
    701,   702,   703,   704,   705,   706,   707,   708,   709,   711,   712,   713,   714,   715,   716,   717,
    718,   719,   720,   722,   723,   724,   725,   726,   727,   728,   729,   730,   732,   733,   734,   735,
    736,   737,   738,   739,   740,   741,   743,   744,   745,   746,   747,   748,   749,   750,   751,   752,
    754,   755,   756,   757,   758,   759,   760,   761,   762,   764,   765,   766,   767,   768,   769,   771,
    773,   775,   776,   778,   780,   781,   783,   785,   787,   788,   790,   792,   794,   795,   797,   799,
    800,   802,   804,   806,   807,   809,   811,   813,   814,   816,   818,   820,   821,   823,   825,   826,
    828,   830,   832,   833,   835,   837,   839,   840,   842,   844,   845,   847,   849,   851,   852,   854,
    856,   858,   859,   861,   863,   864,   866,   868,   870,   871,   873,   875,   877,   878,   880,   882,
    884,   885,   887,   889,   890,   892,   894,   896,   898,   901,   903,   906,   909,   911,   914,   917,
    920,   922,   925,   928,   931,   933,   936,   939,   941,   944,   947,   950,   952,   955,   958,   960,
    963,   966,   969,   971,   974,   977,   980,   982,   985,   988,   990,   993,   996,   999,   1001,  1004,
    1007,  1010,  1012,  1015,  1018,  1020,  1023,  1027,  1031,  1036,  1040,  1045,  1050,  1054,  1059,  1063,
    1068,  1072,  1077,  1082,  1086,  1091,  1095,  1100,  1104,  1109,  1114,  1118,  1123,  1127,  1132,  1136,
    1141,  1146,  1150,  1156,  1163,  1170,  1177,  1184,  1192,  1199,  1206,  1213,  1220,  1227,  1234,  1241,
    1248,  1256,  1263,  1270,  1277,  1286,  1298,  1310,  1321,  1333,  1344,  1356,  1368,  1379,  1391,  1403,
    1419,  1440,  1462,  1483,  1504,  1526,  1552,  1584,  1616,  1648,  1686,  1728,  1771,  1856,  1984,  2047};