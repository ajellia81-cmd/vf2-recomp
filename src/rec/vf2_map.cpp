// The address map (made by tools/recomp/vf2recomp map): addresses only.
#include "i960_analyse.h"
const char i960rec::vf2_map_text[] = R"MAP(# Virtua Fighter 2 (Model 2A) i960 code map for the recompiler (src/rec/i960_analyse.h): where
# the functions start, where the indirect jumps and calls lead, and the calls that return past
# data. Addresses only, nothing from the ROM. Made by tools/recomp/vf2recomp from vf2run --cov runs.
thunk 0093c4
thunk 0093c8
thunk 0093cc
thunk 0093d0
thunk 0093d4
thunk 0093d8
thunk 0093dc
thunk 0093e0
thunk 0093e4
thunk 0093e8
thunk 0093ec
thunk 0093f0
thunk 0093f4
thunk 0093f8
thunk 0093fc
thunk 009400
thunk 009404
thunk 009408
thunk 009444
thunk 04ec00
thunk 0502a4
entry 0000b0
entry 0001b0
entry 000530
entry 0006e8
entry 00073c
entry 000754
entry 0007f0
entry 0007fc
entry 0008e0
entry 000b48
entry 000b58
entry 000b6c
entry 000bc0
entry 000d50
entry 000e10
entry 000e60
entry 000ea4
entry 000ed0
entry 000f08
entry 000f0c
entry 000f7c
entry 000fa0
entry 001064
entry 001200
entry 001290
entry 0012bc
entry 0012d8
entry 001314
entry 001344
entry 001610
entry 00175c
entry 001920
entry 001abc
entry 001e6c
entry 001edc
entry 001f5c
entry 0020f0
entry 00245c
entry 002548
entry 002584
entry 002640
entry 0026b0
entry 0026ec
entry 00281c
entry 0028d4
entry 002a10
entry 002b4c
entry 002c38
entry 002de4
entry 002ec4
entry 002edc
entry 002f5c
entry 007b18
entry 007c60
entry 007d14
entry 007d6c
entry 007dc4
entry 007ef0
entry 007f24
entry 007f7c
entry 007fc0
entry 007ff0
entry 008154
entry 0084c8
entry 008918
entry 008954
entry 008980
entry 008eb0
entry 008ef0
entry 008f1c
entry 008fe0
entry 0091e8
entry 009294
entry 00932c
entry 0093e0
entry 0093f0
entry 009444
entry 009480
entry 0094d0
entry 00a048
entry 00a0c4
entry 00a154
entry 00a178
entry 00a3d4
entry 00a6c0
entry 00a748
entry 00a804
entry 00a974
entry 00ab0c
entry 00acf8
entry 00ae78
entry 00b394
entry 00b3f8
entry 00b588
entry 00b66c
entry 00b9b8
entry 00baec
entry 00bc10
entry 00bcb0
entry 00bce4
entry 00bdc8
entry 00c0a4
entry 00c268
entry 00c414
entry 00c448
entry 00c45c
entry 00c474
entry 00c7ec
entry 00cf04
entry 00d014
entry 00d380
entry 00d4ac
entry 00d844
entry 00d918
entry 00d94c
entry 00db80
entry 00db98
entry 00de64
entry 00deb0
entry 00df38
entry 00df60
entry 00e474
entry 00e544
entry 00e590
entry 00e5a8
entry 00eb44
entry 00edf4
entry 00f0f4
entry 00f61c
entry 00f980
entry 00fa50
entry 00fae0
entry 00fcf4
entry 010084
entry 010088
entry 01018c
entry 010218
entry 0102a4
entry 010374
entry 010404
entry 010490
entry 010548
entry 010564
entry 010754
entry 010784
entry 010898
entry 010bc0
entry 010cbc
entry 010d54
entry 010ea4
entry 010ee4
entry 010f08
entry 0110b0
entry 0110f4
entry 01128c
entry 0113f4
entry 011560
entry 011704
entry 011744
entry 0117a8
entry 0117f8
entry 011a8c
entry 011b48
entry 011be4
entry 011c20
entry 011c44
entry 011c78
entry 013f08
entry 01442c
entry 014640
entry 0146ec
entry 01499c
entry 01645c
entry 016504
entry 0176a0
entry 017710
entry 01791c
entry 017b68
entry 0180bc
entry 018144
entry 018644
entry 018a54
entry 018ac8
entry 018b14
entry 018b58
entry 018bd4
entry 018c64
entry 018d44
entry 018e00
entry 018e08
entry 018ef8
entry 0199e0
entry 019a7c
entry 019ae0
entry 019c94
entry 019d28
entry 019e34
entry 019ebc
entry 019ef0
entry 019ef8
entry 01a138
entry 01a1e4
entry 01ab34
entry 01ab74
entry 01b534
entry 01b568
entry 01b5c8
entry 01b654
entry 01b668
entry 01b694
entry 01b74c
entry 01b9ac
entry 01bac8
entry 01c5fc
entry 01c6cc
entry 01c6ec
entry 01c70c
entry 01d320
entry 01e52c
entry 01ee34
entry 01eff0
entry 01f03c
entry 01f144
entry 01f148
entry 01f1c0
entry 01f24c
entry 01f254
entry 01f40c
entry 01facc
entry 01fbb4
entry 01fc00
entry 01fcc0
entry 01fee4
entry 01ff0c
entry 01fffc
entry 020064
entry 0202e8
entry 020400
entry 020488
entry 020558
entry 020660
entry 02069c
entry 020708
entry 020c0c
entry 020d34
entry 020de8
entry 020ed4
entry 021180
entry 021220
entry 021368
entry 0214dc
entry 021550
entry 0216b8
entry 02173c
entry 021864
entry 021904
entry 0219d8
entry 021bec
entry 021c08
entry 0221cc
entry 0221e8
entry 022298
entry 0223bc
entry 022404
entry 0225bc
entry 0225cc
entry 0230d4
entry 023238
entry 0233d0
entry 023524
entry 02364c
entry 023694
entry 023878
entry 0238a4
entry 0238f8
entry 02396c
entry 023bfc
entry 023ee8
entry 023f30
entry 023f6c
entry 024158
entry 024230
entry 0242a4
entry 024314
entry 0244d0
entry 0244d4
entry 024534
entry 024680
entry 02468c
entry 024d8c
entry 024da4
entry 024dbc
entry 024f30
entry 024f3c
entry 024f90
entry 026ef0
entry 0270d4
entry 027130
entry 02776c
entry 027b5c
entry 027ce0
entry 028184
entry 028780
entry 028848
entry 028918
entry 02901c
entry 029414
entry 02954c
entry 029598
entry 029648
entry 029744
entry 029748
entry 02ab78
entry 02ab94
entry 02b180
entry 02b194
entry 02b1bc
entry 02b1f8
entry 02b27c
entry 02b950
entry 02b9fc
entry 02bf50
entry 02c5f0
entry 02c770
entry 02c894
entry 02c8ac
entry 02c8c4
entry 02c8dc
entry 02c8f4
entry 02c90c
entry 02c924
entry 02c944
entry 02c968
entry 02c9b4
entry 02cae0
entry 02cba0
entry 02cc8c
entry 02ccd0
entry 02ce24
entry 02ce28
entry 02ce40
entry 02ce80
entry 02cefc
entry 02cfa0
entry 02cfa4
entry 02d014
entry 02d040
entry 02d048
entry 02d090
entry 02d174
entry 02d1bc
entry 02db04
entry 02db88
entry 02db8c
entry 02db90
entry 02eaa0
entry 02eab0
entry 02eab8
entry 02ec24
entry 02eed0
entry 02f988
entry 02fa14
entry 02fb58
entry 02fcbc
entry 02fd78
entry 02ff74
entry 030168
entry 03018c
entry 030328
entry 0303e8
entry 030500
entry 030910
entry 030950
entry 03096c
entry 030a70
entry 030b2c
entry 030b8c
entry 030ca4
entry 030e74
entry 030eac
entry 031004
entry 031040
entry 0311b8
entry 0313d8
entry 031408
entry 031810
entry 031adc
entry 031c68
entry 031ca8
entry 031e80
entry 031e9c
entry 031eac
entry 031fb8
entry 032034
entry 032090
entry 0320cc
entry 0324a0
entry 0334a8
entry 0334f8
entry 034d94
entry 034db0
entry 0355e0
entry 035e0c
entry 035e28
entry 036f2c
entry 036f48
entry 03912c
entry 039148
entry 0399cc
entry 039c40
entry 039d0c
entry 039e34
entry 039f08
entry 039f60
entry 03a2e0
entry 03a728
entry 03a7c0
entry 03a824
entry 03a9a8
entry 03aadc
entry 03ac10
entry 03ace8
entry 03aed0
entry 03af9c
entry 03b130
entry 03b1dc
entry 03b278
entry 03b378
entry 03b930
entry 03d06c
entry 03d0e4
entry 03d15c
entry 03d228
entry 03d3e0
entry 03d44c
entry 03d4b8
entry 03d678
entry 03dde0
entry 03df0c
entry 03e044
entry 03e158
entry 03e2a8
entry 03e3ec
entry 03e530
entry 03e674
entry 03e7a0
entry 03e890
entry 03e9c8
entry 03eaf4
entry 03eb68
entry 03ebb8
entry 03ec08
entry 03ec70
entry 03ecc8
entry 03ed24
entry 03ed5c
entry 03ed88
entry 03ee0c
entry 03ee78
entry 03eef0
entry 03ef50
entry 03f070
entry 03f0ec
entry 03f118
entry 03f148
entry 03f208
entry 03f2f8
entry 03f3b4
entry 03f41c
entry 03f46c
entry 03f4b0
entry 03f4f4
entry 03f558
entry 03f5d0
entry 03f630
entry 03f664
entry 03f6b8
entry 03f708
entry 03f734
entry 03f79c
entry 03f7e4
entry 03f834
entry 03f890
entry 03f904
entry 03f978
entry 03fa08
entry 03fa90
entry 03fb10
entry 03fbac
entry 03fbc8
entry 03fe64
entry 0401cc
entry 04036c
entry 040880
entry 040914
entry 040a10
entry 040a98
entry 040c40
entry 040e60
entry 040f80
entry 041160
entry 0411ec
entry 0411f4
entry 041284
entry 041310
entry 041398
entry 041850
entry 041878
entry 041c14
entry 041da4
entry 041f28
entry 042168
entry 0422d4
entry 042824
entry 04287c
entry 042a1c
entry 042cfc
entry 042d90
entry 042ed0
entry 042ed4
entry 042fbc
entry 0430e0
entry 043104
entry 0431a0
entry 0431f0
entry 0432d4
entry 04332c
entry 0434f4
entry 04372c
entry 0437bc
entry 0437d0
entry 043888
entry 043950
entry 0439ac
entry 0439fc
entry 043a40
entry 043abc
entry 043fd0
entry 044084
entry 044268
entry 04ad40
entry 04afb4
entry 04b020
entry 04b410
entry 04b44c
entry 04b488
entry 04b4ac
entry 04b570
entry 04b5b0
entry 04b5d0
entry 04b604
entry 04b640
entry 04b7b0
entry 04b7e8
entry 04b820
entry 04b838
entry 04b8d8
entry 04b914
entry 04b934
entry 04b9b8
entry 04ba5c
entry 04ba70
entry 04ba80
entry 04bb18
entry 04bcd4
entry 04bfe0
entry 04c180
entry 04c928
entry 04c9dc
entry 04cb64
entry 04cd18
entry 04ce88
entry 04cfc4
entry 04d16c
entry 04d25c
entry 04d2c0
entry 04e7b4
entry 04e808
entry 04ec00
entry 04ec0c
entry 04eda0
entry 04effc
entry 04f0a4
entry 04f0d8
entry 04f124
entry 04f1ec
entry 04f214
entry 04f220
entry 04f24c
entry 04f330
entry 04f61c
entry 04f644
entry 04f734
entry 04f774
entry 04f7c4
entry 04f854
entry 04f904
entry 04f944
entry 04fb00
entry 050130
entry 05015c
entry 0501f8
entry 0502a4
entry 0502c0
entry 0503b4
entry 0504a4
entry 05052c
entry 0505d4
entry 0508c4
entry 051d3c
entry 051f98
entry 0520d0
entry 052378
entry 052518
entry 05271c
entry 052774
entry 05284c
entry 052974
entry 052bc8
entry 052e7c
entry 05304c
entry 053750
entry 0537bc
entry 053810
entry 053884
entry 0538e4
entry 053cb8
entry 053f30
entry 053fb0
entry 053ff0
entry 0541f4
entry 054240
entry 05427c
entry 054318
entry 054370
entry 054380
entry 05451c
entry 054820
entry 0548cc
entry 0549e8
entry 054be8
entry 054c14
entry 056ee0
entry 057bdc
entry 05a460
entry 05ff2c
entry 05ff54
entry 05ff7c
entry 06001c
entry 06010c
entry 060330
entry 06116c
entry 061198
entry 061684
entry 061a14
entry 061ac4
entry 061b88
entry 061ba0
entry 062fac
entry 063828
entry 0640f0
entry 0640f4
entry 06428c
entry 064864
entry 064ca0
entry 064e5c
entry 0651bc
entry 065318
entry 06534c
entry 065384
entry 0653f0
entry 0655f8
entry 0657dc
entry 065838
entry 06ad90
entry 06aec4
entry 06af94
entry 06afe0
entry 06b074
entry 06b130
entry 06b19c
entry 06b2e8
entry 06b410
entry 06b50c
entry 06b57c
entry 06b624
entry 06b650
entry 06b6f0
entry 06bf64
entry 06bf98
entry 06ca64
entry 06ca84
entry 06cae0
entry 06caf0
entry 06caf4
entry 06cb04
entry 06cb2c
entry 06cb70
entry 06cbb4
entry 06d10c
entry 06d144
entry 06d17c
entry 06d1b4
entry 06d1ec
entry 06d270
entry 06d418
entry 06d518
entry 06d590
entry 06d7c8
entry 06d85c
entry 06d8b4
entry 06d958
entry 06d970
entry 06d9f8
entry 06dcb8
entry 06dd4c
entry 06e1cc
entry 009f9c
entry 00a1a0
entry 00a1bc
entry 00a1f8
entry 00a234
entry 00a2f0
entry 00a32c
entry 00a368
entry 00a3a4
entry 00ba0c
entry 00c0f8
entry 06df2c
entry 014714
entry 014720
entry 014760
entry 014778
entry 014798
entry 0147bc
entry 014950
entry 014984
entry 0149b8
entry 0149ec
entry 014a88
entry 014b54
entry 014be4
entry 014c74
entry 014d10
entry 014dac
entry 014dc4
entry 014e78
entry 014f04
entry 01502c
entry 0151e0
entry 015308
entry 01564c
entry 0157dc
entry 015dc8
entry 015e28
entry 015f04
entry 016030
entry 016330
entry 0185ec
entry 0185fc
entry 019218
entry 019530
entry 019544
entry 0195f8
entry 01a398
entry 01a408
entry 01a420
entry 01a554
entry 01a5c4
entry 01a760
entry 01a7f0
entry 01a828
entry 01a840
entry 01a8d4
entry 01a8f0
entry 01a9d8
entry 01aa18
entry 01aa30
entry 01ab04
entry 01ab0c
entry 01ab2c
entry 01adac
entry 01adc4
entry 01af18
entry 01af88
entry 01afa0
entry 01afd0
entry 01b05c
entry 01b0c8
entry 01b23c
entry 01b25c
entry 01b290
entry 01b2b0
entry 01b2d0
entry 01b2f4
entry 01b368
entry 01b398
entry 01b3c4
entry 01b400
entry 01c820
entry 01c84c
entry 01c87c
entry 01c8ac
entry 01c8dc
entry 01c8e8
entry 01c918
entry 01c950
entry 01c974
entry 01c988
entry 01c9c4
entry 01c9d8
entry 01c9ec
entry 01ca08
entry 01ca54
entry 01ca9c
entry 01cab8
entry 01cac4
entry 01caf4
entry 01cb34
entry 01cb54
entry 01cb74
entry 01cbf0
entry 01cc74
entry 01cc88
entry 01ccc4
entry 01cd00
entry 01cd14
entry 01cd84
entry 01ce04
entry 01ce24
entry 01ce38
entry 01ce7c
entry 01cf44
entry 01cfac
entry 01cfe8
entry 01d034
entry 01fc44
entry 01fc78
entry 020a44
entry 020a50
entry 020a60
entry 020a74
entry 021c58
entry 021ce4
entry 021e9c
entry 021ce0
entry 021d08
entry 021fd8
entry 02fbac
entry 02fbb0
entry 02fbf8
entry 030a0c
entry 030a18
entry 030a24
entry 030a50
entry 043b80
entry 043b98
entry 043bfc
entry 043d4c
entry 043d68
entry 04c7c8
entry 04c854
entry 04c89c
entry 04c798
entry 054c4c
entry 054cc0
entry 054d00
entry 054d68
entry 064cc0
entry 064d60
entry 064df0
entry 064e4c
entry 06b878
entry 06bbcc
entry 06bcd4
entry 06bd10
entry 06be1c
entry 01ad98
entry 01c7f0
entry 043b50
entry 054c3c
edge 000b54 000200
edge 000b54 000208
edge 000b54 000214
edge 000b54 000220
edge 000b68 000170
edge 000b68 00018c
edge 00947c 009f9c
edge 00947c 00a1a0
edge 00947c 00a1bc
edge 00947c 00a1f8
edge 00947c 00a234
edge 00947c 00a2f0
edge 00947c 00a32c
edge 00947c 00a368
edge 00947c 00a3a4
edge 00947c 00b640
edge 00947c 00ba0c
edge 00947c 00bd9c
edge 00947c 00c0f8
edge 00947c 02db84
edge 00947c 063850
edge 00947c 06df2c
edge 00947c 06df6c
edge 00a6f0 000d50
edge 00a6f0 00a804
edge 00a6f0 00a974
edge 00a6f0 00ab0c
edge 00a6f0 00acf8
edge 00a6f0 00c45c
edge 00a6f0 00c474
edge 00a6f0 00c7ec
edge 00a6f0 00d014
edge 00a6f0 00d380
edge 00a6f0 010088
edge 00a6f0 01018c
edge 00a6f0 0102a4
edge 00a6f0 010404
edge 00a6f0 010784
edge 00a6f0 010898
edge 00a6f0 010bc0
edge 00ad24 00ae78
edge 00ad24 00b394
edge 00ad24 00b3f8
edge 00ad24 00b4f0
edge 00ad24 00b588
edge 00ad24 00b66c
edge 00ad24 00b9b8
edge 00ad24 00baec
edge 00ad24 00bc10
edge 00ad24 00bcb0
edge 00ad24 00bce4
edge 00ad24 00bdc8
edge 00ad24 00c0a4
edge 00ad24 00c268
edge 00ad24 00c414
edge 00ad24 00c448
edge 00d4a4 00d4ac
edge 00d4a4 00d844
edge 00d4a4 00d94c
edge 00d4a4 00db80
edge 00d4a4 00db98
edge 00d4a4 00de64
edge 00d4a4 00deb0
edge 00d4a4 00df38
edge 00d4a4 00df60
edge 00d4a4 00e474
edge 00d4a4 00e544
edge 00d4a4 00e590
edge 00d4a4 00e5a8
edge 00d4a4 00eb44
edge 00d4a4 00edf4
edge 00d4a4 00f0f4
edge 00d4a4 00f61c
edge 00d4a4 00f85c
edge 00d4a4 00f980
edge 00d4a4 00fa50
edge 00d4a4 00fae0
edge 00d4a4 00fcf4
edge 00d760 013f08
edge 00d770 013f08
edge 010478 010490
edge 010478 010548
edge 010478 010564
edge 010478 010754
edge 010dc8 000d50
edge 010dc8 000e10
edge 010dc8 013f08
edge 010dc8 0142f4
edge 010dc8 01645c
edge 010dc8 01b9ac
edge 010dc8 01b9dc
edge 010dc8 01d320
edge 010dc8 01d458
edge 010dc8 0221cc
edge 010dc8 0221e8
edge 010dc8 029748
edge 010dc8 02b1bc
edge 010dc8 02b1f8
edge 010dc8 02eaa0
edge 010dc8 02eab0
edge 010dc8 031810
edge 010dc8 031adc
edge 010dc8 032090
edge 010dc8 0320cc
edge 010dc8 0324a0
edge 010dc8 039c40
edge 010dc8 039c6c
edge 010dc8 039d0c
edge 010dc8 039d5c
edge 010dc8 03a728
edge 010dc8 03a76c
edge 010dc8 03a824
edge 010dc8 03a830
edge 010dc8 03aadc
edge 010dc8 03aed0
edge 010dc8 03af9c
edge 010dc8 03b038
edge 010dc8 03b0a0
edge 010dc8 03b130
edge 010dc8 03b1dc
edge 010dc8 03d06c
edge 010dc8 03d0e4
edge 010dc8 03d3e0
edge 010dc8 03d44c
edge 010dc8 0439fc
edge 010dc8 043abc
edge 010dc8 051d3c
edge 010dc8 051e7c
edge 010dc8 0640f4
edge 010dc8 06428c
edge 010dc8 0657dc
edge 010dc8 06ca64
edge 010dc8 06cae0
edge 010dc8 06caf0
edge 010dc8 06caf4
edge 010dc8 06cb04
edge 010e90 02b1f8
edge 014700 000d50
edge 014700 014714
edge 014700 014720
edge 014700 014760
edge 014700 014778
edge 014700 014798
edge 014700 0147a4
edge 014700 0147bc
edge 014700 014950
edge 014700 014984
edge 0149b0 0149b8
edge 0149b0 0149ec
edge 0149b0 014a88
edge 0149b0 014b54
edge 0149b0 014be4
edge 0149b0 014c74
edge 0149b0 014d10
edge 0149b0 014dac
edge 0149b0 014dc4
edge 0149b0 014e78
edge 0149b0 014f04
edge 0149b0 01502c
edge 0149b0 0151e0
edge 0149b0 015308
edge 0149b0 01564c
edge 0149b0 0157dc
edge 0149b0 015dc8
edge 0149b0 015e28
edge 0149b0 015f04
edge 0149b0 016030
edge 0149b0 016330
edge 0185cc 0185ec
edge 0185cc 0185f0
edge 0185cc 0185fc
edge 0191d4 000d50
edge 0191d4 019218
edge 0191d4 019530
edge 0191d4 019544
edge 0191d4 0195f8
edge 0191d4 019694
edge 01a34c 01a398
edge 01a34c 01a39c
edge 01a34c 01a408
edge 01a34c 01a420
edge 01a34c 01a554
edge 01a34c 01a5c4
edge 01a34c 01a760
edge 01a34c 01a7f0
edge 01a34c 01a828
edge 01a34c 01a840
edge 01a34c 01a8d4
edge 01a34c 01a8f0
edge 01a34c 01a9d8
edge 01a34c 01aa18
edge 01a34c 01aa30
edge 01a34c 01ab04
edge 01a34c 01ab0c
edge 01a34c 01ab2c
edge 01ad94 01adac
edge 01ad94 01adc4
edge 01ad94 01af18
edge 01ad94 01af88
edge 01ad94 01afa0
edge 01ad94 01afd0
edge 01ad94 01b05c
edge 01ad94 01b0c8
edge 01ad94 01b23c
edge 01ad94 01b25c
edge 01ad94 01b290
edge 01ad94 01b2b0
edge 01ad94 01b2d0
edge 01ad94 01b2f4
edge 01ad94 01b368
edge 01ad94 01b398
edge 01ad94 01b3c4
edge 01ad94 01b400
edge 01c110 01bfac
edge 01c6c8 01bd1c
edge 01c6c8 01c274
edge 01c754 01c3a0
edge 01c754 01c474
edge 01c7ec 01c820
edge 01c7ec 01c84c
edge 01c7ec 01c87c
edge 01c7ec 01c8ac
edge 01c7ec 01c8dc
edge 01c7ec 01c8e8
edge 01c7ec 01c918
edge 01c7ec 01c950
edge 01c7ec 01c974
edge 01c7ec 01c988
edge 01c7ec 01c9c4
edge 01c7ec 01c9d8
edge 01c7ec 01c9ec
edge 01c7ec 01ca08
edge 01c7ec 01ca54
edge 01c7ec 01ca9c
edge 01c7ec 01cab8
edge 01c7ec 01cac4
edge 01c7ec 01caf4
edge 01c7ec 01cb34
edge 01c7ec 01cb54
edge 01c7ec 01cb74
edge 01c7ec 01cbf0
edge 01c7ec 01cc74
edge 01c7ec 01cc88
edge 01c7ec 01ccc4
edge 01c7ec 01cd00
edge 01c7ec 01cd14
edge 01c7ec 01cd84
edge 01c7ec 01ce04
edge 01c7ec 01ce24
edge 01c7ec 01ce38
edge 01c7ec 01ce7c
edge 01c7ec 01cf44
edge 01c7ec 01cfac
edge 01c7ec 01cfe8
edge 01c7ec 01d034
edge 01cbb0 034d94
edge 01cbb0 034db0
edge 01cbb0 0355e0
edge 01cbb0 035e0c
edge 01cbb0 035e28
edge 01cbb0 036f2c
edge 01cbb0 036f48
edge 01cbb0 03912c
edge 01cbb0 039148
edge 01cbb0 0399cc
edge 01d190 01bf34
edge 01d190 01c0ec
edge 01d190 01c154
edge 01d190 01c3a0
edge 01d190 01c3c8
edge 01d190 01c458
edge 01d190 01c474
edge 01d190 01c740
edge 01d1a8 01ab34
edge 01d4d4 000d50
edge 01d4d4 000e10
edge 01d4d4 01f144
edge 01d4d4 01f148
edge 01d4d4 01f1c0
edge 01d4d4 021bec
edge 01d4d4 021c08
edge 01d4d4 06ad90
edge 01d4d4 06aec4
edge 01d4d4 06b19c
edge 01d4d4 06b27c
edge 01d4d4 06b2e8
edge 01d4d4 06b410
edge 01d4d4 06b50c
edge 01d4d4 06b57c
edge 01d4d4 06b650
edge 01d4d4 06b6f0
edge 01fc30 000d50
edge 01fc30 01fc44
edge 01fc30 01fc78
edge 01fc30 01fca8
edge 020a20 020a44
edge 020a20 020a50
edge 020a20 020a60
edge 020a20 020a74
edge 021c38 021c58
edge 021c38 021ce4
edge 021c38 021e9c
edge 021c54 021ce0
edge 021c54 021d08
edge 021c54 021fd8
edge 0223f8 02244c
edge 0225c8 000d50
edge 0225c8 022428
edge 0225c8 022434
edge 023874 000d50
edge 023874 023648
edge 0244c8 0244d0
edge 0244c8 0244d4
edge 024678 024680
edge 024678 02468c
edge 024678 024d8c
edge 024678 024da4
edge 024678 024dbc
edge 024678 024f30
edge 02b190 02abf4
edge 02b190 02ac20
edge 02b190 02ac4c
edge 02b1a4 02ac6c
edge 02b1a4 02ac74
edge 02b1a4 02accc
edge 02b1a4 02ace0
edge 02b1a4 02acf0
edge 02b1a4 02ad48
edge 02b1a4 02ad5c
edge 02b1a4 02ad6c
edge 02b1a4 02adbc
edge 02b1a4 02add0
edge 02b1a4 02adf8
edge 02b1a4 02ae0c
edge 02b1a4 02ae48
edge 02b1a4 02ae6c
edge 02b1a4 02ae90
edge 02b1a4 02ae98
edge 02b1a4 02aea0
edge 02b1a4 02aea8
edge 02b1a4 02aed8
edge 02b1a4 02af10
edge 02b1a4 02af84
edge 02b1a4 02af8c
edge 02b1a4 02af98
edge 02b1a4 02afa0
edge 02b1a4 02afb8
edge 02b1a4 02afc0
edge 02b1a4 02b01c
edge 02b1a4 02b024
edge 02b1a4 02b030
edge 02b1a4 02b038
edge 02b1a4 02b050
edge 02b1a4 02b060
edge 02b1a4 02b0b8
edge 02b1a4 02b0c0
edge 02b1a4 02b0c8
edge 02b1a4 02b0f8
edge 02b1a4 02b10c
edge 02b1a4 02b114
edge 02b1a4 02b11c
edge 02b1a4 02b154
edge 02b274 02c968
edge 02b274 02ccd0
edge 02b274 02ce24
edge 02b274 02ce28
edge 02b274 02ce40
edge 02b274 02ce80
edge 02b274 02cefc
edge 02b274 02cfa0
edge 02b274 02cfa4
edge 02b274 02d014
edge 02b274 02d040
edge 02b274 02d048
edge 02b274 02d090
edge 02b274 02d0fc
edge 02b274 02d174
edge 02b274 02d1bc
edge 02b274 02d550
edge 02b274 02db04
edge 02b274 02db88
edge 02b274 02db8c
edge 02b274 02db90
edge 02fb98 02fbac
edge 02fb98 02fbb0
edge 02fbec 02fbf8
edge 03093c 030950
edge 03093c 03096c
edge 03093c 030a70
edge 030990 030a0c
edge 030990 030a18
edge 030990 030a24
edge 030990 030a50
edge 03a1b8 013f08
edge 03a1c8 013f08
edge 03a820 03a798
edge 03a820 03a7a8
edge 03aecc 03a954
edge 03aecc 03a974
edge 03aecc 03ab14
edge 03aecc 03ab2c
edge 03bd40 06b7a0
edge 03d53c 03dde0
edge 03d53c 03df0c
edge 03d53c 03e044
edge 03d53c 03e158
edge 03d53c 03e2a8
edge 03d53c 03e3ec
edge 03d53c 03e530
edge 03d53c 03e674
edge 03d53c 03e7a0
edge 03d53c 03e890
edge 03d53c 03e9c8
edge 03d550 03fe64
edge 03d550 0401cc
edge 03d550 04036c
edge 03d550 040880
edge 03d550 040914
edge 03d550 040a10
edge 03d550 040a98
edge 03d550 040c40
edge 03d550 040e60
edge 03d550 0411ec
edge 03d550 0411f4
edge 03d550 041284
edge 03d550 041398
edge 03d550 041878
edge 03d550 041da4
edge 03d550 041f28
edge 03d550 042168
edge 03d550 0422d4
edge 03d550 042824
edge 03d550 042ed0
edge 03d550 042ed4
edge 03d550 042fbc
edge 03d5e4 03dde0
edge 03d5e4 03df0c
edge 03d5e4 03e044
edge 03d5e4 03e158
edge 03d5e4 03e2a8
edge 03d5e4 03e3ec
edge 03d5e4 03e674
edge 03d5e4 03e7a0
edge 03d5e4 03e890
edge 03d5e4 03e9c8
edge 03d5f8 03fe64
edge 03d5f8 0401cc
edge 03d5f8 04036c
edge 03d5f8 040a10
edge 03d5f8 040a98
edge 03d5f8 040c40
edge 03d5f8 040e60
edge 03d5f8 040f80
edge 03d5f8 041160
edge 03d5f8 0411ec
edge 03d5f8 0411f4
edge 03d5f8 041284
edge 03d5f8 041310
edge 03d5f8 041398
edge 03d5f8 041850
edge 03d5f8 041878
edge 03d5f8 041c14
edge 03d5f8 041da4
edge 03d5f8 041f28
edge 03d5f8 042168
edge 03d5f8 0422d4
edge 03d5f8 042824
edge 03d5f8 042ed0
edge 03d5f8 042ed4
edge 03d5f8 042fbc
edge 042834 04287c
edge 042834 042a1c
edge 042834 042cfc
edge 042834 042d90
edge 043b4c 000d50
edge 043b4c 043b80
edge 043b4c 043b98
edge 043b4c 043bfc
edge 043b4c 043c98
edge 043d44 043d4c
edge 043d44 043d68
edge 04af88 04be7c
edge 04af88 04beb8
edge 04af88 04bef0
edge 04af88 04bf28
edge 04af88 04c720
edge 04af88 04cb28
edge 04af88 04cc88
edge 04af88 04cdd0
edge 04af88 04cf20
edge 04c794 000bc0
edge 04c794 000d50
edge 04c794 04c7c8
edge 04c794 04c7dc
edge 04c794 04c854
edge 04c794 04c89c
edge 04c838 000bc0
edge 04c838 000d50
edge 04c838 04c798
edge 04c838 04c7c8
edge 04c838 04c7dc
edge 04c838 04c854
edge 04c838 04c89c
edge 052514 05219c
edge 052770 052618
edge 0528f0 0525a8
edge 053880 051df0
edge 053880 051e04
edge 0538e0 0521a0
edge 05443c 054360
edge 05443c 05437c
edge 054c38 054c4c
edge 054c38 054cc0
edge 054c38 054d00
edge 054c38 054d68
edge 06450c 000bc0
edge 06450c 000d50
edge 06450c 000e10
edge 06450c 064864
edge 06450c 064ca0
edge 06450c 064e5c
edge 06450c 0651bc
edge 06450c 065318
edge 064cac 000d50
edge 064cac 064cc0
edge 064cac 064d60
edge 064cac 064df0
edge 064cac 064e4c
edge 0653bc 000d50
edge 0653bc 000e10
edge 0653bc 0640f0
edge 0653bc 0653f0
edge 06b7c0 06b878
edge 06b7c0 06b940
edge 06b7c0 06bbcc
edge 06b7c0 06bc48
edge 06b7c0 06bcd4
edge 06b7c0 06bd10
edge 06b7c0 06bd4c
edge 06b7c0 06bd54
edge 06b7c0 06be1c
edge 06b7c0 06bea0
edge 06cab4 000e10
edge 06cab4 06cae0
edge 06cab4 06caf0
edge 06cab4 06caf4
edge 06cab4 06cb04
)MAP";
