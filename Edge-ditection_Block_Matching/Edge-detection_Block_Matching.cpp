#include <string>
#include<sstream> //文字ストリーム
#include<fstream>
#include<iostream>
#include <windows.h>
#include <stdio.h>
#include <vector>

#include <tuple>
#include <direct.h>//フォルダを作成する

using namespace std;

//メモリ確保を行うためのヘッダ
#define ANSI				
#include "nrutil.h"	

char *EdBM_name1_s = "Angle.csv";
//char *EdBM_name2_s = "threshold2.csv";
char *EdBM_name2_s = "edge_st.csv";

char *EdBM_name1t_s = "Anglet.csv";
//char *EdBM_name2t_s = "threshold2t.csv";
char *EdBM_name2t_s = "edge_st_t.csv";

char inputAngle_directory[255];

char inputAngle_deta[128];
char inputAnglet_deta[128];
char inputthreshold2_deta[128];
char inputthreshold2t_deta[128];

//std::tuple<int, int, std::vector<std::vector<double>>>read_csv(const char *filename);
int write_frame(char date_directory[], char Inputiamge[],  std::vector<int> max_x, std::vector<int> max_y, int image_xt, int image_yt, int count_tied_V_vote, int V_vote_max);
int msectime();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////最大のV_voteとその座標を求める////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::tuple<std::vector<int>, std::vector<int>, double, int> max_v_vote_calculate(double **V_vote, int image_x, int image_y, int image_xt, int image_yt) {

	std::vector<int> max_x;
	std::vector<int> max_y;
	int hajime_count = 0;
	double max_V = 0;
	int count_tied_V_vote = 0;
	max_V = V_vote[0][0];

	for (int y = 0; y < image_y - image_yt; y++) {
		for (int x = 0; x < image_x - image_xt; x++) {

			if (V_vote[x][y] > max_V) {
				hajime_count = 1;
				count_tied_V_vote = 1;
				max_x.resize(count_tied_V_vote);
				max_y.resize(count_tied_V_vote);

				max_V = V_vote[x][y];
				max_x[0] = x;
				max_y[0] = y;
				//printf("V[%d][%d]=%f,", x, y, V[x][y]);
			}
			if (V_vote[x][y] == max_V &&hajime_count != 1) {
				max_x.resize(count_tied_V_vote + 1);
				max_y.resize(count_tied_V_vote + 1);
				max_x[count_tied_V_vote] = x;
				max_y[count_tied_V_vote] = y;
				++count_tied_V_vote;
			}

			hajime_count = 0;
		}
	}

	hajime_count = 0;
	return std::forward_as_tuple(max_x, max_y, count_tied_V_vote, max_V);

}

std::tuple<std::vector<int>, std::vector<int>, int, int> Edge_detection_Block_Matching(char date_directory[], int &image_x, int &image_y, int &image_xt, int &image_yt, int paramerter[], int paramerter_count, int sd, char date[],int Bs, double threshold_EdBM, char Inputiamge[], double &threshold_otsu,int &frame_allowable_error, double &use_threshold_flag) {
	printf("\n****************************************\n");
	printf("start：Edge-detection_Block_Matching\n");
	printf("****************************************\n");
	
	//Nrutilを用いたメモリの確保
	double **Angle = matrix(0, image_x - 1, 0, image_y - 1);
	double **threshold2 = matrix(0, image_x - 1, 0, image_y - 1);
	double **Anglet = matrix(0, image_xt - 1, 0, image_y - 1);
	double **threshold2t = matrix(0, image_xt - 1, 0, image_yt - 1);

	double **threshold_tMAP = matrix(0, image_xt - 1, 0, image_yt - 1);

	//ブロックの数
	int M, N;
	M = image_xt / Bs;
	N = image_yt / Bs;

	int bm = 0;
	int bn = 0;

	double **CB = matrix(0, image_x - image_xt- 1, 0, image_y - image_yt - 1);
	double **V_vote = matrix(0, image_x - image_xt - 1, 0, image_y - image_yt - 1);

	double min_CB;
	int min_x, min_y;
	

	//確保したメモリを初期化する
	for (int i = 0; i < image_y; i++) {
		for (int j = 0; j < image_x; j++) {
			Angle[j][i] = 0;
			threshold2[j][i] = 0;
		}
	}
	for (int i = 0; i < image_yt; i++) {
		for (int j = 0; j < image_xt; j++) {
			Anglet[j][i] = 0;
			threshold2t[j][i] = 0;
			threshold_tMAP[j][i] = 0;
		}
	}

	for (int i = 0; i < image_y - image_yt; i++) {
		for (int j = 0; j < image_x - image_xt; j++) {
			CB[j][i] = 0;
			V_vote[j][i] = 0;
		}
	}

	
	
	//inputデータの選択(cossimのみ)
	switch (paramerter[0]){
	case 3:
		sprintf(inputAngle_directory, "%s%dk_cossim_sd%d\\", date_directory, paramerter[paramerter_count], sd);
		break;
	case 4:
	case 5:
		sprintf(inputAngle_directory, "%s%d×%dsobel_atan_sd%d\\", date_directory, paramerter[paramerter_count], paramerter[paramerter_count], sd);
		break;
	}

	//Inputファイルのディレクトリ設定
	sprintf(inputAngle_deta, "%s%s", inputAngle_directory, EdBM_name1_s);
	sprintf(inputAnglet_deta, "%s%s", inputAngle_directory, EdBM_name1t_s);
	sprintf(inputthreshold2_deta, "%s%s", inputAngle_directory, EdBM_name2_s);
	sprintf(inputthreshold2t_deta, "%s%s", inputAngle_directory, EdBM_name2t_s);

	//Inputファイルを開く
	ifstream Angle_f(inputAngle_deta);
	ifstream Anglet_f(inputAnglet_deta);
	ifstream threshold2_f(inputthreshold2_deta);
	ifstream threshold2t_f(inputthreshold2t_deta);

	if (!Angle_f) { printf("入力エラー Angle.csvがありません_Edge-detection_Block_Matching_filename=%s", inputAngle_deta); exit(1); }
	if (!Anglet_f) { printf("入力エラー Anglet.csvがありません_Edge-detection_Block_Matching_filename=%s", inputAnglet_deta); exit(1); }
	if (!threshold2_f) { printf("入力エラー threshold2.csvがありません_Edge-detection_Block_Matching_filename=%s", inputthreshold2_deta); exit(1); }
	if (!threshold2t_f) { printf("入力エラー threshold2t.csvがありません_Edge-detection_Block_Matching_filename=%s", inputthreshold2t_deta); exit(1); }
	
	int count_large = 0;
	int count_small = 0;
/////////データの読み込み(対象画像)////////////////////////////////////////////////////////////////////////
	int i = 1;
	printf("対象画像の結果を読み取ります\n");
	string str_Angle, str_threshold2;
	count_large = 0;
	while (getline(Angle_f, str_Angle)) {					//このループ内ですべてを行う
		count_small = 0;			//初期化
///////////////いろいろ定義．ここでやらないといけない///////////////////////////////////////////////////////////////////////////
		string token_Angle_f;
		istringstream stream_Angle(str_Angle);

		getline(threshold2_f, str_threshold2);	string token_threshold2_f;	istringstream stream_threshold2(str_threshold2);

//////////////////////////配列に代入//////////////////////////////////////////////////////////////////////////////////////////////////
		while (getline(stream_Angle, token_Angle_f, ',')) {	//一行読み取る．V0のみ，繰り返しの範囲指定に用いる
			double tmp_Angle = stof(token_Angle_f);			//文字を数字に変換
			Angle[count_small][count_large] = tmp_Angle;				//配列に代入

			getline(stream_threshold2, token_threshold2_f, ',');
			double tmp_threshold2 = stof(token_threshold2_f);			//文字を数字に変換
			threshold2[count_small][count_large] = tmp_threshold2;				//配列に代入
			++count_small;
			}
			++count_large;
		}
/////////データの読み込み(テンプレート画像)////////////////////////////////////////////////////////////////////////
	i = 1;
	printf("テンプレート画像の結果を読み取ります\n");
	string str_Anglet, str_threshold2t;
	count_large = 0;
	while (getline(Anglet_f, str_Anglet)) {					//このループ内ですべてを行う
		count_small = 0;			//初期化
									///////////////いろいろ定義．ここでやらないといけない///////////////////////////////////////////////////////////////////////////
		string token_Anglet_f;
		istringstream stream_Anglet(str_Anglet);

		getline(threshold2t_f, str_threshold2t);	string token_threshold2t_f;	istringstream stream_threshold2t(str_threshold2t);

//////////////////////////配列に代入//////////////////////////////////////////////////////////////////////////////////////////////////
		while (getline(stream_Anglet, token_Anglet_f, ',')) {	//一行読み取る．V0のみ，繰り返しの範囲指定に用いる
			double tmp_Anglet = stof(token_Anglet_f);			//文字を数字に変換
			Anglet[count_small][count_large] = tmp_Anglet;				//配列に代入

			getline(stream_threshold2t, token_threshold2t_f, ',');
			double tmp_threshold2t = stof(token_threshold2t_f);			//文字を数字に変換
			threshold2t[count_small][count_large] = tmp_threshold2t;				//配列に代入
			++count_small;
		}
		++count_large;
	}

///////////////Edge-detection_Block_Matching//////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	//arctanの場合は判別分析法で求めた閾値を用いる
	//threshold_EdBM = threshold_otsu;
	
	
	if(use_threshold_flag==0)threshold_EdBM = 0;
	if (use_threshold_flag == 1)threshold_EdBM = threshold_otsu;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//thresholdのflag
	for (int i = 0; i < image_yt; i++) {
		for (int j = 0; j < image_xt; j++) {
			if (threshold2t[j][i] >= threshold_EdBM) {

				threshold_tMAP[j][i] = 1;

				if(j>0 && i>0)threshold_tMAP[j - 1][i - 1] = 1;
				if (i>0)threshold_tMAP[j][i - 1] = 1;
				if (j<image_xt - 1 && i>0)threshold_tMAP[j + 1][i - 1] = 1;

				if(j>0)threshold_tMAP[j - 1][i] = 1;
				if (j<image_xt - 1)threshold_tMAP[j + 1][i] = 1;

				if(j>0 && i<image_yt-1)threshold_tMAP[j - 1][i + 1] = 1;
				if (i < image_yt - 1)threshold_tMAP[j][i + 1] = 1;
				if(j<image_xt - 1 && i<image_yt - 1)threshold_tMAP[j + 1][i + 1] = 1;

			}
		}
	}

//CB
	
	//特定のブロックについて
	for (int n = 0; n < N; n++) {
		for (int m = 0; m < M; m++) {

			bm = Bs*m;
			bn = Bs*n;
			min_CB = 0;
			min_x = 0;
			min_y = 0;
			for (int y = 0; y < image_y - image_yt; y++) {
				for (int x = 0; x < image_x - image_xt; x++) {
					CB[x][y] = 0;
				}
			}


			//座標x,yを比べる
			for (int y = 0; y < image_y - image_yt; y++) {
				for (int x = 0; x < image_x - image_xt; x++) {

					for (int k = 0; k < Bs; k++) {
						for (int l = 0; l < Bs; l++) {
							if (threshold_tMAP[bm + l][bn + k] = 1) {
								CB[x][y] += abs(Angle[x + bm + l][y + bn + k] - Anglet[bm + l][bn + k]);
							}
							else {
								CB[x][y] += 0;
							}

						}
					}

				}
			}
			
			//m,nについてCBが最小となるx,yを求める
			min_CB = CB[0][0];
			for (int y = 0;  y< image_y - image_yt; y++) {
				for (int x = 0; x < image_x - image_xt; x++) {
					if (CB[x][y] < min_CB) {
						min_CB = CB[x][y];
						min_x = x;
						min_y = y;
					}
				}
			}
			//printf("x=%d,y=%d,%f\n", min_x, min_y,min_CB);
			V_vote[min_x][min_y] += 1;
			
		}
	}


	double **V_vote2 = matrix(0, image_x - image_xt - 1, 0, image_y - image_yt - 1);
	//初期化
	for (int i = 0; i < image_y - image_yt; i++) {
		for (int j = 0; j < image_x - image_xt; j++) {
			V_vote2[j][i] = 0;
		}
	}

	double max_V = 0;
	int count_tied_V_vote = 0;

	std::vector<int>max_x;
	std::vector<int>max_y;
	//最大のV_voteの座標とその値を求める
	std::tie(max_x, max_y, count_tied_V_vote, max_V) = max_v_vote_calculate(V_vote, image_x, image_y, image_xt, image_yt);


	//複数の枠を統合する
	//int frame_allowable_error = 5;
	if (frame_allowable_error != 0 && count_tied_V_vote != 1) {
		//全ブロックに対して
		//±frame_allowable_errorの範囲を取る
		for (int i = 0; i < count_tied_V_vote + 1; ++i) {
			for (int k = -frame_allowable_error; k < frame_allowable_error + 1; ++k) {
				for (int l = -frame_allowable_error; l < frame_allowable_error + 1; ++l) {
					if (max_y[i] + l >= 0 && max_y[i] + l < image_y - image_yt) {
						if (max_x[i] + k >= 0 && max_x[i] + k < image_x - image_xt) {
							V_vote2[max_x[i]][max_y[i]] += V_vote[max_x[i] + k][max_y[i] + l];
						}
					}
				}
			}
		}
		//v_voteに入れなおす
		for (int i = 0; i < image_y - image_yt; i++) {
			for (int j = 0; j < image_x - image_xt; j++) {
				V_vote[j][i] = V_vote2[j][i];
			}
		}


		free_matrix(V_vote2, 0, image_x - image_xt - 1, 0, image_y - image_yt - 1);
		max_V = 0;
		count_tied_V_vote = 0;
		int hajime_count = 0;

		max_V = V_vote[0][0];
		max_x.resize(1);
		max_y.resize(1);
		std::tie(max_x, max_y, count_tied_V_vote, max_V) = max_v_vote_calculate(V_vote, image_x, image_y, image_xt, image_yt);

		int count_tied_number = 0;
		if (count_tied_V_vote != 1) {
			for (int i = 0; i < count_tied_V_vote + 1; ++i) {

				if (max_x[i] - max_x[0] <= 2 * frame_allowable_error && max_y[i] - max_y[0] <= 2 * frame_allowable_error) {
					max_x[i] = max_x[0];
					max_y[i] = max_y[0];
					++count_tied_number;
					//ここでcount_tied_V_voteを変更する
					if (i = count_tied_V_vote && count_tied_number == count_tied_V_vote + 1) {
						count_tied_V_vote = 1;
						max_x.resize(count_tied_V_vote);
						max_y.resize(count_tied_V_vote);
					}
				}

			}
		}
	}
	/*
	double max_V=0;
	int hajime_count = 0;
	int count_tied_V_vote = 0;
	std::vector<int> max_x;
	std::vector<int> max_y;//求める座標
	//int max_x = 0, max_y = 0;
	max_V = V[0][0];
	/*
	for (int y = 0; y < image_y - image_yt; y++) {
		for (int x = 0; x < image_x - image_xt; x++) {
			
			if (V[x][y] > max_V) {
				max_V = V[x][y];
				max_x = x;
				max_y = y;
				//printf("V[%d][%d]=%f,", x, y, V[x][y]);
			}

		}
	}
	
	for (int y = 0; y < image_y - image_yt; y++) {
		for (int x = 0; x < image_x - image_xt; x++) {

			if (V[x][y] > max_V) {
				hajime_count = 1;
				count_tied_V_vote = 1;
				max_x.resize(count_tied_V_vote);
				max_y.resize(count_tied_V_vote);

				max_V = V[x][y];
				max_x[0] = x;
				max_y[0] = y;
				//printf("V[%d][%d]=%f,", x, y, V[x][y]);
			}
			if (V[x][y] == max_V &&hajime_count != 1) {
				max_x.resize(count_tied_V_vote + 1);
				max_y.resize(count_tied_V_vote + 1);
				max_x[count_tied_V_vote] = x;
				max_y[count_tied_V_vote] = y;
				++count_tied_V_vote;
			}

			hajime_count = 0;
		}
	}
	*/
	printf("max_x=%d,max_y=%d\n", max_x, max_y);
	

	//write_frame(date_directory, Inputiamge, max_x, max_y, image_xt, image_yt);
	write_frame(date_directory, Inputiamge, max_x, max_y, image_xt, image_yt, count_tied_V_vote, max_V);
//////////////////////////////logの作成///////////////////////////////////////////////////////////////////////////////////
	FILE *fp_date_Matching;
	char filename_log_Matching[128];
	//sprintf(filename_log, "..\\log\\log-%2d-%02d%02d-%02d%02d%02d.txt",pnow->tm_year+1900,pnow->tm_mon + 1,pnow->tm_mday,pnow->tm_hour,pnow->tm_min,pnow->tm_sec);	//logファイル作成のディレクトリ指定
	sprintf(filename_log_Matching, "%s\\log_Matching.txt", date_directory);	//logファイル作成のディレクトリ指定
	if ((fp_date_Matching = fopen(filename_log_Matching, "w")) == NULL) { printf("logファイルが開けません"); exit(1); }
	for (int i = 0; i < count_tied_V_vote; ++i) {
		fprintf(fp_date_Matching, "領域の座標：(x,y)=(%d,%d),(%d,%d)\n", max_x[i], max_y[i], max_x[i] + image_xt, max_y[i] + image_yt);
	}
	fprintf(fp_date_Matching, "ブロックサイズ：x=%d,y=%d\n", image_xt, image_yt); 
	fprintf(fp_date_Matching, "threshold_otsu=%lf\n", threshold_otsu);
	if (threshold_EdBM == threshold_otsu) {
		fprintf(fp_date_Matching, "判別分析法を用いた：threshold_otsu=%lf\n", threshold_otsu);
	}
	else {
		fprintf(fp_date_Matching, "自分で設定した閾値を用いた：threshold_EdBM=%lf\n", threshold_EdBM);
	}
	fprintf(fp_date_Matching, "ヒストグラムに成分0を表示していません\n閾値を求めるためには用いています\n");
	
	fclose(fp_date_Matching);
	printf("logファイル %s を保存しました\n", filename_log_Matching);


	//メモリの解放
	free_matrix(Angle, 0, image_x - 1, 0, image_y - 1);
	free_matrix(threshold2, 0, image_x - 1, 0, image_y - 1);
	free_matrix(Anglet, 0, image_xt - 1, 0, image_yt - 1);
	free_matrix(threshold2t, 0, image_xt - 1, 0, image_yt - 1);
	free_matrix(threshold_tMAP, 0, image_xt - 1, 0, image_yt - 1);
	free_matrix(CB, 0, image_x - image_xt - 1, 0, image_y - image_yt - 1);
	free_matrix(V_vote, 0, image_x - image_xt - 1, 0, image_y - image_yt - 1);

	

	
	return std::forward_as_tuple(max_x, max_y, count_tied_V_vote, max_V);
}