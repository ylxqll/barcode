/*
 * Code128_fill.cpp
 *
 *  Created on: 2015年4月22日
 *      Author: welkinm
 */


#include <stdio.h>
#include <string.h>
#include "Code128_fill.h"


const char* Code128A_Get_Code(char c) {
	if(c>95 || c<0) return NULL;
	if(c>=32) {
		return CODE128_ENCODING[c-32].encoding;
	}
	else {
		return CODE128_ENCODING[c+64].encoding;
	}
}

const char* Code128B_Get_Code(char c) {
	if(c>127 || c<32) return NULL;
	return CODE128_ENCODING[c-32].encoding;
}

const char* Code128C_Get_Code(char *str) {
	if(str==NULL) return NULL;
	if(str[0]<48 || str[0]>57) return NULL;
	if(str[1]<48 || str[1]>57) return NULL;
	return CODE128_ENCODING[(str[0]-48)*10+str[1]-48].encoding;
}

char Code128A_Get_Idx(char c) {
	if(c>=32) {
		return c-32;
	}
	else {
		return c+64;
	}
}
char Code128B_Get_Idx(char c) {
	return c-32;
}
char Code128C_Get_Idx(char *str) {
	return (str[0]-48)*10+str[1]-48;
}

int Code128_Fill_Char(BarCode_BMPHead_Type &head, unsigned char *buf, unsigned int bufLen, const char *code, int codeLen,
		int &xIdx, int thickness, int h, BarCode_BMRGBQuad_Type &barRGB) {
	char count = 0;
	for(int i=0; i<codeLen; i++) {
		count = code[i]-48;
		if((i&1)==0) {
			BarCode_BMP_Mem_Fill_Rect(head, buf, xIdx, 0, thickness*count, h, barRGB);
		}
		xIdx += thickness*count;
	}
	return 0;
}

int Code128B_Fill_Buf(const char *barcode, unsigned int barcodeLen, unsigned char *buf, unsigned int bufLen,
		unsigned int& w, unsigned int& h, unsigned int& bmpLen, bool isColorExchange) {
	//设置BMP文件头信息
	BarCode_BMPHead_Type head;
	int ret = BarCode_BMP_Build_Normal_Head(&head, w, h);
	if(ret!=0) return 1;
	//计算X轴基本间隙长度，整个条码一共有 10 + 11 + barcodeLen*11 + 11 + 13 + 10
	unsigned int miniWidth = 10 + 11 + barcodeLen*11 + 11 + 13 + 10;
	printf("miniWidth:%d\n", miniWidth);
	if(w<miniWidth) return 2;
	int thickness = w/miniWidth;			//基本条的宽度
	//写入BMP头信息
	ret = BarCode_BMP_Mem_Write_Head(&head, buf, bufLen);
	if(ret!=0) return 1;
	bmpLen = head.fh.bfSize;
	//背景色与线条色
	BarCode_BMRGBQuad_Type bgRGB;
	BarCode_BMRGBQuad_Type barRGB;
	if(isColorExchange) {
		barRGB.rgbRed = barRGB.rgbGreen = barRGB.rgbBlue = 0xFF;
		bgRGB.rgbRed = bgRGB.rgbGreen = bgRGB.rgbBlue = 0;
	} else {
		bgRGB.rgbRed = bgRGB.rgbGreen = bgRGB.rgbBlue =  0xFF;
		barRGB.rgbRed = barRGB.rgbGreen = barRGB.rgbBlue = 0;
	}
	ret = BarCode_BMP_Mem_Wrire_BK_Color(head, buf, bgRGB);
	//生成code128 B 编码
	unsigned int j = 0;
	unsigned int checkSum = 0;
	//定义起始的条X坐标
	int xBegin = thickness * 10;
	//当前条X坐标的索引
	int xIdx = xBegin;
	//第1个为 StartB ，索引104
	Code128_Fill_Char(head, buf, bufLen, CODE128_ENCODING[104].encoding, 6, xIdx, thickness, h, barRGB);
	checkSum += 104;
	for(j=0; j<barcodeLen; ) {
		Code128_Fill_Char(head, buf, bufLen, Code128B_Get_Code(barcode[j]), 6, xIdx, thickness, h, barRGB);
		checkSum += (barcode[j]-32)*(++j);
//		printf("%d\n", checkSum);
	}
	checkSum %= 103;
//	printf("%d\n", checkSum);
	Code128_Fill_Char(head, buf, bufLen, CODE128_ENCODING[checkSum].encoding,  6, xIdx,thickness, h, barRGB);
	Code128_Fill_Char(head, buf, bufLen, CODE128_ENCODING[106].encoding,  7, xIdx,thickness, h, barRGB);
	return 0;
}

char Code128_Get_Char_Type(char c, char currentType) {
	if(c<32) return 'A';
	if(96<=c && c<=127) return 'B';
	if(48<=c && c<=57) return 'C';
	//当之前的类型为C，现类型不为C时，默认返回A
	if(currentType=='C') return 'A';
	return currentType;
}

int Code128_Parse(const char *barcode, int len, char* codeStr, int &codeStrLen, int &codeLen, int &checkSum, char defaultType) {
	int i = 0;
	char currentType = defaultType;
	codeStrLen = 0;
	checkSum = 0;
	const char *str = NULL;
	char c;
	char c2[2];
	//填充头
	switch(defaultType) {
		case 'A': {
			checkSum += 103;
			str = CODE128_ENCODING[103].encoding;
			break;
		}
		case 'B': {
			checkSum += 104;
			str = CODE128_ENCODING[104].encoding;
			break;
		}
		case 'C': {
			checkSum += 105;
			str = CODE128_ENCODING[105].encoding;
			break;
		}
		default:
			return 1;
	}
	memcpy(&codeStr[codeStrLen], str, 6);
	codeStrLen += 6;
	//解析
	int codeIdx = 0;
	char type = 0;
	while(i<len) {
		c = barcode[i];
		type = Code128_Get_Char_Type(c, currentType);
		//填充头
		switch(currentType) {
			case 'A': {
				switch(type) {
					case 'A': {
						//如果之前类型是A，现类型也是A，则直接填充数据
						str = Code128A_Get_Code(barcode[i]);
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						c = Code128A_Get_Idx(barcode[i]);
						checkSum += c*(++codeIdx);
						i++;
						break;
					}
					case 'B': {
						currentType = 'B';
						//如果之前是A类，现类型为B，需要从A切换B，编码为100
						str = CODE128_ENCODING[100].encoding;
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						checkSum += 100*(++codeIdx);
						//填充数据，注意现在己经是B编码的
						str = Code128B_Get_Code(barcode[i]);
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						c = Code128B_Get_Idx(barcode[i]);
						checkSum += c*(++codeIdx);
						i++;
						break;
					}
					case 'C': {
						//如果之前是A类，现类型为C，需查看连续的数字个数，如果大于4个，需从A切换为C
						int j = i;
						while(type=='C' && j<len) {
							type = Code128_Get_Char_Type(barcode[j], currentType);
							j++;
						}
						if(j!=len) {
							j--;	//因为上面的循环结束时，j多加了1
						}
						if((j-i)>3) {
							currentType = 'C';
							//如果大于3个，需从A切换为C，编码为99
							str = CODE128_ENCODING[99].encoding;
							memcpy(&codeStr[codeStrLen], str, 6);
							codeStrLen += 6;
							checkSum += 99*(++codeIdx);
							//成对的填充
							while((j-i)>1) {
								c2[0] = barcode[i++];
								c2[1] = barcode[i++];
								str = Code128C_Get_Code(c2);
								memcpy(&codeStr[codeStrLen], str, 6);
								codeStrLen += 6;
								c = Code128C_Get_Idx(c2);
								checkSum += c*(++codeIdx);
							}
							//检查是否有单位数的，如果存在单位的，智能切换到下一个编码类型
							if((j-i)==1) {
								//查看是否为条码结尾
								if(j==len) {
									currentType = 'A';
									//己到条码结尾，默认从C切成A类型，编码为101
									str = CODE128_ENCODING[101].encoding;;
									memcpy(&codeStr[codeStrLen], str, 6);
									codeStrLen += 6;
									checkSum += 101*(++codeIdx);
									//填充最后一个数字
									str = Code128A_Get_Code(barcode[i]);
									memcpy(&codeStr[codeStrLen], str, 6);
									codeStrLen += 6;
									c = Code128A_Get_Idx(barcode[i]);
									checkSum += c*(++codeIdx);
									i++;
								} else {
									//如果后面还有字符，先检查下一个编码类型，该字符类型一定不为C
									currentType = Code128_Get_Char_Type(barcode[j], currentType);
									if(currentType=='A') {
										//从C切成A类型，编码为101
										str = CODE128_ENCODING[101].encoding;;
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										checkSum += 101*(++codeIdx);
										//填充最后一个数字
										str = Code128A_Get_Code(barcode[i]);
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										c = Code128A_Get_Idx(barcode[i]);
										checkSum += c*(++codeIdx);
										i++;
									}
									else if(currentType=='B') {
										//从C切成B类型，编码为100
										str = CODE128_ENCODING[100].encoding;;
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										checkSum += 100*(++codeIdx);
										//填充最后一个数字
										str = Code128B_Get_Code(barcode[i]);
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										c = Code128B_Get_Idx(barcode[i]);
										checkSum += c*(++codeIdx);
										i++;
									}
								}
							}
						} else {
							//如果不大于3个，则无需切换编码类型，逐个填充
							while(i<j) {
								str = Code128A_Get_Code(barcode[i]);
								memcpy(&codeStr[codeStrLen], str, 6);
								codeStrLen += 6;
								c = Code128A_Get_Idx(barcode[i]);
								checkSum += c*(++codeIdx);
								i++;
							}
						}
						break;
					}
					default:
						return 1;
				}
				break;
			}
			case 'B': {
				switch(type) {
					case 'A': {
						currentType = 'A';
						//如果之前是B类，现类型为A，需要从B切换A，编码为101
						str = CODE128_ENCODING[101].encoding;
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						checkSum += 101*(++codeIdx);
						//填充数据，注意现在己经是A编码的
						str = Code128A_Get_Code(barcode[i]);
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						c = Code128A_Get_Idx(barcode[i]);
						checkSum += c*(++codeIdx);
						i++;
						break;
					}
					case 'B': {
						//如果之前类型是B，现类型也是B，则直接填充数据
						str = Code128B_Get_Code(barcode[i]);
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						c = Code128B_Get_Idx(barcode[i]);
						checkSum += c*(++codeIdx);
						i++;
						break;
					}
					case 'C': {
						//如果之前是A类，现类型为C，需查看连续的数字个数，如果大于4个，需从B切换为C
						int j = i;
						while(type=='C' && j<len) {
							type = Code128_Get_Char_Type(barcode[j], currentType);
							j++;
						}
						if(j!=len) {
							j--;	//因为上面的循环结束时，j多加了1
						}
						if((j-i)>3) {
							currentType = 'C';
							//如果大于3个，需从B切换为C，编码为99
							str = CODE128_ENCODING[99].encoding;
							memcpy(&codeStr[codeStrLen], str, 6);
							codeStrLen += 6;
							checkSum += 99*(++codeIdx);
							//成对的填充
							while((j-i)>1) {
								c2[0] = barcode[i++];
								c2[1] = barcode[i++];
								str = Code128C_Get_Code(c2);
								memcpy(&codeStr[codeStrLen], str, 6);
								codeStrLen += 6;
								c = Code128C_Get_Idx(c2);
								checkSum += c*(++codeIdx);
							}
							//检查是否有单位数的，如果存在单位的，智能切换到下一个编码类型
							if((j-i)==1) {
								//查看是否为条码结尾
								if(j==len) {
									currentType = 'A';
									//己到条码结尾，默认从C切成A类型，编码为101
									str = CODE128_ENCODING[101].encoding;;
									memcpy(&codeStr[codeStrLen], str, 6);
									codeStrLen += 6;
									checkSum += 101*(++codeIdx);
									//填充最后一个数字
									str = Code128A_Get_Code(barcode[i]);
									memcpy(&codeStr[codeStrLen], str, 6);
									codeStrLen += 6;
									c = Code128A_Get_Idx(barcode[i]);
									checkSum += c*(++codeIdx);
									i++;
								} else {
									//如果后面还有字符，先检查下一个编码类型，该字符类型一定不为C
									currentType = Code128_Get_Char_Type(barcode[j], currentType);
									if(currentType=='A') {
										//从C切成A类型，编码为101
										str = CODE128_ENCODING[101].encoding;;
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										checkSum += 101*(++codeIdx);
										//填充最后一个数字
										str = Code128A_Get_Code(barcode[i]);
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										c = Code128A_Get_Idx(barcode[i]);
										checkSum += c*(++codeIdx);
										i++;
									}
									else if(currentType=='B') {
										//从C切成B类型，编码为100
										str = CODE128_ENCODING[100].encoding;;
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										checkSum += 100*(++codeIdx);
										//填充最后一个数字
										str = Code128B_Get_Code(barcode[i]);
										memcpy(&codeStr[codeStrLen], str, 6);
										codeStrLen += 6;
										c = Code128B_Get_Idx(barcode[i]);
										checkSum += c*(++codeIdx);
										i++;
									}
								}
							}
						} else {
							//如果不大于3个，则无需切换编码类型，逐个填充
							while(i<j) {
								str = Code128B_Get_Code(barcode[i]);
								memcpy(&codeStr[codeStrLen], str, 6);
								codeStrLen += 6;
								c = Code128B_Get_Idx(barcode[i]);
								checkSum += c*(++codeIdx);
								i++;
							}
						}
						break;
					}
				}
				break;
			}
			case 'C': {
				switch(type) {
					case 'A': {
						currentType = 'A';
						//从C切成A类型，编码为101
						str = CODE128_ENCODING[101].encoding;;
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						checkSum += 101*(++codeIdx);
						//填充
						str = Code128A_Get_Code(barcode[i]);
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						c = Code128A_Get_Idx(barcode[i]);
						checkSum += c*(++codeIdx);
						i++;
						break;
					}
					case 'B': {
						//从C切成B类型，编码为100
						str = CODE128_ENCODING[100].encoding;;
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						checkSum += 100*(++codeIdx);
						//填充
						str = Code128B_Get_Code(barcode[i]);
						memcpy(&codeStr[codeStrLen], str, 6);
						codeStrLen += 6;
						c = Code128B_Get_Idx(barcode[i]);
						checkSum += c*(++codeIdx);
						i++;
						break;
					}
				}
				break;
			}
			default:
				return 1;
		}
	}
	//计算较验和
	checkSum %= 103;
	//填充较验码
	str = CODE128_ENCODING[checkSum].encoding;
	memcpy(&codeStr[codeStrLen], str, 6);
	codeStrLen += 6;
	//填充结尾码
	str = CODE128_ENCODING[106].encoding;
	memcpy(&codeStr[codeStrLen], str, 7);
	codeStrLen += 7;
	codeStr[codeStrLen] = '\0';
	codeLen = 1+codeIdx+1+1;
	return 0;
}

int Code128B_Auto_Fill_Buf(const char *barcode, unsigned int barcodeLen, unsigned char *buf, unsigned int bufLen,
		unsigned int& w, unsigned int& h, unsigned int& bmpLen, bool isColorExchange) {
	//设置BMP文件头信息
	BarCode_BMPHead_Type head;
	int ret = BarCode_BMP_Build_Normal_Head(&head, w, h);
	if(ret!=0) return 1;
	//解析条码
	char codeStr[2048];
	int codeStrLen = 0;
	int codeLen = 0;
	int checkSum = 0;
	ret = Code128_Parse(barcode, barcodeLen, codeStr, codeStrLen, codeLen, checkSum, 'B');
	if(ret!=0) return ret;
	printf("codeStr:%s\n", codeStr);

	//计算需要最小的位图宽象素数
	unsigned int miniWidth = 10 + (codeLen-1)*11 + 13 + 10;
	printf("miniWidth:%d\n", miniWidth);
	if(w<miniWidth) return 2;
	int thickness = w/miniWidth;			//基本条的宽度

	//写入BMP头信息
	ret = BarCode_BMP_Mem_Write_Head(&head, buf, bufLen);
	if(ret!=0) return 1;
	bmpLen = head.fh.bfSize;
	//背景色与线条色
	BarCode_BMRGBQuad_Type bgRGB;
	BarCode_BMRGBQuad_Type barRGB;
	if(isColorExchange) {
		barRGB.rgbRed = barRGB.rgbGreen = barRGB.rgbBlue = 0xFF;
		bgRGB.rgbRed = bgRGB.rgbGreen = bgRGB.rgbBlue = 0;
	} else {
		bgRGB.rgbRed = bgRGB.rgbGreen = bgRGB.rgbBlue =  0xFF;
		barRGB.rgbRed = barRGB.rgbGreen = barRGB.rgbBlue = 0;
	}
	ret = BarCode_BMP_Mem_Wrire_BK_Color(head, buf, bgRGB);

	//定义起始的条X坐标
	int xBegin = thickness * 10;
	//当前条X坐标的索引
	int xIdx = xBegin;
	//
	int j;
	for(j=0; j<codeLen-1; j++) {
		Code128_Fill_Char(head, buf, bufLen, &codeStr[j*6], 6, xIdx, thickness, h, barRGB);
	}
	Code128_Fill_Char(head, buf, bufLen, &codeStr[j*6], 7, xIdx,thickness, h, barRGB);
	return 0;
}

int Code128_Parse(const char *barcode, int len) {
	return 0;
}
