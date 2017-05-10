#include<iostream>
#include<cstring>
#include<cstdio>
#include<map>
#include<vector>
#include<string>
#include<ctime>
#include<algorithm>
#include<cmath>
#include<cstdlib>

#include <fstream>

using namespace std;

//Ruobing Xie
//Image-embodied Knowledge Representation Learning
//IKRL (ATT), Test for image-based representation

bool debug=false;
bool L1_flag=1;

int n = 50;		//dimension

string version;
string trainortest = "test";
//for vector
map<int,string> id2entity,id2relation,id2type,id2domain;
map<string,int> relation2id,entity2id,type2id,domain2id;		//<relation,ID>
int relation_num,entity_num,type_num,domain_num;

vector<int> image_count_vec;
vector<vector<vector<double> > > image_vec;		//ent_num*img_num*img_dim, image feature vector
vector<vector<double> > image_att_vec;		//ent_num*img_dim, combined image_vec for an entity via mean/max/
vector<vector<double> > image_proj_vec;		//ent_num*ent_dim, projection of image_vec in entity spaceattention

vector<vector<double> > image_mat;		//ent_dim*img_dim, image(4096) -> entity(n)
//for counting
map<string,string> mid2name,mid2type;
map<int,map<int,int> > entity2num;
map<int,int> e2num;
map<pair<string,string>,map<string,double> > rel_left,rel_right;

double sigmod(double x)
{
    return 1.0/(1+exp(-x));
}

double vec_len(vector<double> a)
{
	double res=0;
	for (int i=0; i<a.size(); i++)
		res+=a[i]*a[i];
	return sqrt(res);
}

void vec_output(vector<double> a)
{
	for (int i=0; i<a.size(); i++)
	{
		cout<<a[i]<<"\t";
		if (i%9==4)
			cout<<endl;
	}
	cout<<"-------------------------"<<endl;
}

double sqr(double x)
{
    return x*x;
}

char buf[100000],buf1[100000];

int my_cmp(pair<double,int> a,pair<double,int> b)
{
    return a.first>b.first;
}

double cmp(pair<int,double> a, pair<int,double> b)
{
	return a.second<b.second;
}

class Test{
    vector<vector<double> > relation_vec,entity_vec;


    vector<int> h,l,r;
    vector<int> fb_h,fb_l,fb_r;
    map<pair<int,int>, map<int,int> > ok;
    double res ;
public:
    void add(int x,int y,int z, bool flag)
    {
    	if (flag)
    	{
        	fb_h.push_back(x);
        	fb_r.push_back(z);
        	fb_l.push_back(y);
        }
        ok[make_pair(x,z)][y]=1;
    }
	
	double norm(vector<double> &a)
    {
        double x = vec_len(a);
        if (x>1)
        for (int ii=0; ii<a.size(); ii++)
                a[ii]/=x;
        return 0;
    }

    int rand_max(int x)
    {
        int res = (rand()*rand())%x;
        if (res<0)
            res+=x;
        return res;
    }
    double len;
	
	//calc triple scores
    double calc_sum(int e1,int e2,int rel)
    {
		//calc sum
        double sum=0;
        if (L1_flag)
        	for (int ii=0; ii<n; ii++)
				sum+=-fabs(image_proj_vec[e2][ii]-image_proj_vec[e1][ii]-relation_vec[rel][ii]);
        else
			for (int ii=0; ii<n; ii++)
				sum+=-sqr(image_proj_vec[e2][ii]-image_proj_vec[e1][ii]-relation_vec[rel][ii]);
        return sum;
    }
	
    void run()
    {
		//relation_vec
        FILE* f1 = fopen(("../res/relation2vec."+version).c_str(),"r");
        cout<<relation_num<<' '<<entity_num<<endl;
        relation_vec.resize(relation_num);
        for (int i=0; i<relation_num;i++)
        {
            relation_vec[i].resize(n);
            for (int ii=0; ii<n; ii++)
                fscanf(f1,"%lf",&relation_vec[i][ii]);
        }
		//entity_vec
		FILE* f2 = fopen(("../res/entity2vec."+version).c_str(),"r");
		entity_vec.resize(entity_num);
        for (int i=0; i<entity_num;i++)
        {
            entity_vec[i].resize(n);
            for (int ii=0; ii<n; ii++)
                fscanf(f2,"%lf",&entity_vec[i][ii]);
        }
		//image_mat
		FILE* f3 = fopen(("../res/image_mat."+version).c_str(),"r");
		image_mat.resize(n);
        for (int i=0; i<n;i++)
        {
            image_mat[i].resize(4096);
			for(int ii=0; ii<4096; ii++)
			{
				fscanf(f3,"%lf",&image_mat[i][ii]);
			}
        }
		
		//init
		//image_att_vec
		image_att_vec.resize(entity_num);
		for(int i=0; i<entity_num; i++)
		{
			image_att_vec[i].resize(4096);
			for(int ii=0; ii<image_count_vec[i]; ii++)
				norm(image_vec[i][ii]);
		}
		//image_proj_vec
		image_proj_vec.resize(entity_num);
		for(int i=0; i<entity_num; i++)
			image_proj_vec[i].resize(n);
		//calc image-based representation
		for(int ent=0; ent<entity_num; ent++)
		{
			//calc image_att_vec with attention
			vector<double> attention_vec, temp_image_proj_vec;
			attention_vec.resize(image_count_vec[ent]);
			temp_image_proj_vec.resize(n);
			for(int i=0; i<image_count_vec[ent]; i++)		//calc attention
			{
				for(int ii=0; ii<n; ii++)		//calc proj_vec
				{
					temp_image_proj_vec[ii] = 0;
					for(int iii=0; iii<4096; iii++)
					{
						temp_image_proj_vec[ii] += image_vec[ent][i][iii] * image_mat[ii][iii];
					}
				}
				attention_vec[i] = 0;
				for(int ii=0; ii<n; ii++)		//calc attention
				{
					attention_vec[i] += temp_image_proj_vec[ii] * entity_vec[ent][ii];
				}
				attention_vec[i] = exp(attention_vec[i]);
			}
			double tempExp = 0;
			for(int i=0; i<image_count_vec[ent]; i++)		//attention norm
				tempExp += attention_vec[i];
			for(int i=0; i<image_count_vec[ent]; i++)		//attention norm
				attention_vec[i] /= tempExp;
			//calc image_att_vec
			for(int i=0; i<4096; i++)
				image_att_vec[ent][i] = 0;
			for(int i=0; i<image_count_vec[ent]; i++)
			{
				for(int ii=0; ii<4096; ii++)
				{
					image_att_vec[ent][ii] += attention_vec[i] * image_vec[ent][i][ii];
				}
			}
			//calc image_proc_vec
			for(int i=0; i<n; i++)
			{
				image_proj_vec[ent][i] = 0;
				for(int ii=0; ii<4096; ii++)
				{
					image_proj_vec[ent][i] += image_att_vec[ent][ii] * image_mat[i][ii];
				}
			}
			/*
			for(int i=0; i<image_count_vec[ent]; i++)
				cout << attention_vec[i] << ' ';
			cout << endl;
			*/
			//cout << vec_len(image_att_vec[ent]) << ' ' << vec_len(image_proj_vec[ent]) << endl;
		}
		
		//test
		double lsum=0 ,lsum_filter= 0;
		double rsum = 0,rsum_filter=0;
		double mid_sum = 0,mid_sum_filter=0;
		double lp_n=0,lp_n_filter = 0;
		double rp_n=0,rp_n_filter = 0;
		double mid_p_n=0,mid_p_n_filter = 0;
		map<int,double> lsum_r,lsum_filter_r;
		map<int,double> rsum_r,rsum_filter_r;
		map<int,double> mid_sum_r,mid_sum_filter_r;
		map<int,double> lp_n_r,lp_n_filter_r;
		map<int,double> rp_n_r,rp_n_filter_r;
		map<int,double> mid_p_n_r,mid_p_n_filter_r;
		map<int,int> rel_num;

		int hit_relation = 1;		//hits@N for relation
		int hit_entity = 10;		//hits@N for entity
		for (int testid = 0; testid<fb_l.size(); testid+=1)
		{
			int h = fb_h[testid];
			int l = fb_l[testid];
			int rel = fb_r[testid];
			double tmp = calc_sum(h,l,rel);
			rel_num[rel]+=1;
			vector<pair<int,double> > a;
			for (int i=0; i<entity_num; i++)		//predict head
			{
				double sum = calc_sum(i,l,rel);
				a.push_back(make_pair(i,sum));
			}
			sort(a.begin(),a.end(),cmp);
			double ttt=0;
			int filter = 0;
			for (int i=a.size()-1; i>=0; i--)
			{
				if (ok[make_pair(a[i].first,rel)].count(l)>0)
					ttt++;
			    if (ok[make_pair(a[i].first,rel)].count(l)==0)
			    	filter+=1;
				if (a[i].first ==h)
				{
					lsum+=a.size()-i;
					lsum_filter+=filter+1;
					lsum_r[rel]+=a.size()-i;
					lsum_filter_r[rel]+=filter+1;
					if (a.size()-i<=hit_entity)
					{
						lp_n+=1;
						lp_n_r[rel]+=1;
					}
					if (filter<hit_entity)
					{
						lp_n_filter+=1;
						lp_n_filter_r[rel]+=1;
					}
					break;
				}
			}
			a.clear();
			for (int i=0; i<entity_num; i++)		//predict tail
			{
				double sum = calc_sum(h,i,rel);
				a.push_back(make_pair(i,sum));
			}
			sort(a.begin(),a.end(),cmp);
			ttt=0;
			filter=0;
			for (int i=a.size()-1; i>=0; i--)
			{

				if (ok[make_pair(h,rel)].count(a[i].first)>0)
					ttt++;
				if (ok[make_pair(h,rel)].count(a[i].first)==0)
			    	filter+=1;
				if (a[i].first==l)
				{
					rsum+=a.size()-i;
					rsum_filter+=filter+1;
					rsum_r[rel]+=a.size()-i;
					rsum_filter_r[rel]+=filter+1;
					if (a.size()-i<=hit_entity)
					{
						rp_n+=1;
						rp_n_r[rel]+=1;
					}
					if (filter<hit_entity)
					{
						rp_n_filter+=1;
						rp_n_filter_r[rel]+=1;
					}
					break;
				}
			}
			a.clear();
			for (int i=0; i<relation_num; i++)		//predict relation
			{
				double sum = calc_sum(h,l,i);
				a.push_back(make_pair(i,sum));
			}
			sort(a.begin(),a.end(),cmp);
			ttt=0;
			filter=0;
			for (int i=a.size()-1; i>=0; i--)
			{
				if (ok[make_pair(h,a[i].first)].count(l)>0)
					ttt++;
				if (ok[make_pair(h,a[i].first)].count(l)==0)
			    	filter+=1;
				if (a[i].first==rel)
				{
					mid_sum+=a.size()-i;
					mid_sum_filter+=filter+1;
					mid_sum_r[rel]+=a.size()-i;
					mid_sum_filter_r[rel]+=filter+1;
					if (a.size()-i<=hit_relation)
					{
						mid_p_n+=1;
						mid_p_n_r[rel]+=1;
					}
					if (filter<hit_relation)
					{
						mid_p_n_filter+=1;
						mid_p_n_filter_r[rel]+=1;
					}
					break;
				}
			}
			if (testid%100==0)
			{
				cout<<testid<<":"<<endl;
				cout<<"left:\t"<<lsum/(testid+1)<<' '<<lp_n/(testid+1)<<' '<<lsum_filter/(testid+1)<<' '<<lp_n_filter/(testid+1)<<endl;
				cout<<"right:\t"<<rsum/(testid+1)<<' '<<rp_n/(testid+1)<<' '<<rsum_filter/(testid+1)<<' '<<rp_n_filter/(testid+1)<<endl;
				cout<<"mid:\t"<<mid_sum/(testid+1)<<' '<<mid_p_n/(testid+1)<<"\t"<<mid_sum_filter/(testid+1)<<' '<<mid_p_n_filter/(testid+1)<<endl;
			}
		}
		//output results
		ofstream fout;
		fout.open("../res.txt");
		fout<<"left:\t"<<lsum/fb_l.size()<<'\t'<<lp_n/fb_l.size()<<"\t"<<lsum_filter/fb_l.size()<<'\t'<<lp_n_filter/fb_l.size()<<endl;
		fout<<"right:\t"<<rsum/fb_r.size()<<'\t'<<rp_n/fb_r.size()<<'\t'<<rsum_filter/fb_r.size()<<'\t'<<rp_n_filter/fb_r.size()<<endl;
		fout<<"mid:\t"<<mid_sum/fb_l.size()<<'\t'<<mid_p_n/fb_l.size()<<"\t"<<mid_sum_filter/fb_l.size()<<'\t'<<mid_p_n_filter/fb_l.size()<<endl;
		for (int rel=0; rel<relation_num; rel++)
		{
			int num = rel_num[rel];
			fout<<"rel:"<<id2relation[rel]<<' '<<num<<endl;
			fout<<"left:"<<lsum_r[rel]/num<<'\t'<<lp_n_r[rel]/num<<"\t"<<lsum_filter_r[rel]/num<<'\t'<<lp_n_filter_r[rel]/num<<endl;
			fout<<"right:"<<rsum_r[rel]/num<<'\t'<<rp_n_r[rel]/num<<'\t'<<rsum_filter_r[rel]/num<<'\t'<<rp_n_filter_r[rel]/num<<endl;
			fout<<"mid:"<<mid_sum_r[rel]/num<<'\t'<<mid_p_n_r[rel]/num<<'\t'<<mid_sum_filter_r[rel]/num<<'\t'<<mid_p_n_filter_r[rel]/num<<endl;
		}
		fout.close();
    }

};
Test test;

void prepare()
{
    FILE* f1 = fopen("../data/entity2id.txt","r");
	FILE* f2 = fopen("../data/relation2id.txt","r");
	FILE* f3 = fopen("../../image_res/image2count.txt","r");
	FILE* f4 = fopen("../../image_res/image2vec_fc7.txt","r");
	int x;
	double y;
	//build entity2ID��ID2entity map
	while (fscanf(f1,"%s%d",buf,&x)==2)
	{
		string st=buf;
		entity2id[st]=x;
		id2entity[x]=st;
		mid2type[st]="None";
		entity_num++;
	}
	//build relation2ID��ID2relation map
	while (fscanf(f2,"%s%d",buf,&x)==2)
	{
		string st=buf;
		relation2id[st]=x;
		id2relation[x]=st;
		relation_num++;
	}
	//build image2count
	image_count_vec.resize(entity_num);
	while (fscanf(f3,"%s%d",buf,&x)==2)
	{
		string st=buf;
		if(entity2id.count(st) == 1)		//in entity list
			image_count_vec[entity2id[st]] = x;
	}
	
	//build image_feature_vec
	image_vec.resize(entity_num);
	for(int i=0; i<entity_num; i++)
		image_vec[i].resize(image_count_vec[i]);
	vector<int> temp_count_vec;
	temp_count_vec.resize(entity_num);
	for(int i=0; i<entity_num; i++)
		temp_count_vec[i] = 0;
	while (fscanf(f4,"%s",buf)==1)
	{
		string st=buf;
		string temp_ent = st.substr(0, st.find_first_of("_"));
		if(entity2id.count(temp_ent) == 1)		//in entity list
		{
			int temp_ent_id =  entity2id[temp_ent];
			int temp_img_count = temp_count_vec[temp_ent_id];
			image_vec[temp_ent_id][temp_img_count].resize(4096);
			for (int ii=0; ii<4096; ii++)
				fscanf(f4,"%lf",&image_vec[temp_ent_id][temp_img_count][ii]);
			temp_count_vec[temp_ent_id]++;
		}
		else {		//not in list, discard
			for (int ii=0; ii<4096; ii++)
				fscanf(f4,"%lf",&y);
		}
	}
	//build triple set
    FILE* f_kb = fopen("../data/test.txt","r");
	while (fscanf(f_kb,"%s",buf)==1)
    {
        string s1=buf;
        fscanf(f_kb,"%s",buf);
        string s2=buf;
        fscanf(f_kb,"%s",buf);
        string s3=buf;
        if (entity2id.count(s1)==0)
        {
            cout<<"miss entity:"<<s1<<endl;
        }
        if (entity2id.count(s2)==0)
        {
            cout<<"miss entity:"<<s2<<endl;
        }
        if (relation2id.count(s3)==0)
        {
        	cout<<"miss relation:"<<s3<<endl;
            relation2id[s3] = relation_num;
            relation_num++;
        }
        test.add(entity2id[s1],entity2id[s2],relation2id[s3],true);
    }
    fclose(f_kb);
    FILE* f_kb1 = fopen("../data/train.txt","r");
	while (fscanf(f_kb1,"%s",buf)==1)
    {
        string s1=buf;
        fscanf(f_kb1,"%s",buf);
        string s2=buf;
        fscanf(f_kb1,"%s",buf);
        string s3=buf;
        if (entity2id.count(s1)==0)
        {
            cout<<"miss entity:"<<s1<<endl;
        }
        if (entity2id.count(s2)==0)
        {
            cout<<"miss entity:"<<s2<<endl;
        }
        if (relation2id.count(s3)==0)
        {
            relation2id[s3] = relation_num;
            relation_num++;
        }

        entity2num[relation2id[s3]][entity2id[s1]]+=1;
        entity2num[relation2id[s3]][entity2id[s2]]+=1;
        e2num[entity2id[s1]]+=1;
        e2num[entity2id[s2]]+=1;
        test.add(entity2id[s1],entity2id[s2],relation2id[s3],false);
    }
    fclose(f_kb1);
    FILE* f_kb2 = fopen("../data/valid.txt","r");
	while (fscanf(f_kb2,"%s",buf)==1)
    {
        string s1=buf;
        fscanf(f_kb2,"%s",buf);
        string s2=buf;
        fscanf(f_kb2,"%s",buf);
        string s3=buf;
        if (entity2id.count(s1)==0)
        {
            cout<<"miss entity:"<<s1<<endl;
        }
        if (entity2id.count(s2)==0)
        {
            cout<<"miss entity:"<<s2<<endl;
        }
        if (relation2id.count(s3)==0)
        {
            relation2id[s3] = relation_num;
            relation_num++;
        }
        test.add(entity2id[s1],entity2id[s2],relation2id[s3],false);
    }
    fclose(f_kb2);
}

int main(int argc,char**argv)
{
    if (argc<2)
        return 0;
    else
    {
        version = argv[1];
        if (argc==3)
            trainortest = argv[2];
        prepare();
        test.run();
    }
}

