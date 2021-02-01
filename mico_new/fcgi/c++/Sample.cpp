#include <iostream>
#include <string>
#include "WXBizDataCrypt.h"

using namespace WxBizDataSecure;
using namespace std;

int main()
{
    cout<<"start to test"<<endl<<endl<<endl;
    string sSessionkey = "EHNNmKX4nvp0GIyifY8DiA==";
    string sAppid = "wxa3376a8cbea27476";    
    WXBizDataCrypt oWXBizDataCrypt(sAppid, sSessionkey);
    
    //decrypt
	string sEncryptedData1=
		"CiyLU1Aw2KjvrjMdj8YKliAjtP4gsMZM"
		"QmRzooG2xrDcvSnxIMXFufNstNGTyaGS"
		"9uT5geRa0W4oTOb1WT7fJlAC+oNPdbB+"
		"3hVbJSRgv+4lGOETKUQz6OYStslQ142d"
		"NCuabNPGBzlooOmB231qMM85d2/fV6Ch"
		"evvXvQP8Hkue1poOFtnEtpyxVLW1zAo6"
		"/1Xx1COxFvrc2d7UL/lmHInNlxuacJXw"
		"u0fjpXfz/YqYzBIBzD6WUfTIF9GRHpOn"
		"/Hz7saL8xz+W//FRAUid1OksQaQx4CMs"
		"8LOddcQhULW4ucetDf96JcR3g0gfRK4P"
		"C7E/r7Z6xNrXd2UIeorGj5Ef7b1pJAYB"
		"6Y5anaHqZ9J6nKEBvB4DnNLIVWSgARns"
		"/8wR2SiRS7MNACwTyrGvt9ts8p12PKFd"
		"lqYTopNHR1Vf7XjfhQlVsAJdNiKdYmYV"
		"oKlaRv85IfVunYzO0IKXsyl7JCUjCpoG"
		"20f0a04COwfneQAGGwd5oa+T8yO5hzuy"
		"Db/XcxxmK01EpqOyuxINew==";

	string sEncryptedData="kCu/V/ngmPSDmUlcjmh9iHx89jG8Nk6BvqSE2WyR/OGH+INDlr4hCY1FYMCO4+bjTg7Z4sTwg0A02fn5LzSPWBgWAfpKjxRxQ+S9jRTVNZWB"
		"rCKCNnhH0916fVD/mlldLE7HOX3FM+ASa1dtQOyDNW79NJ/ZZpleE0/yON7YvGfEGF/ziWe0v90qsRwe1QDdF0jYVw27B+NqtL4bbEaICA==";
	string sIv = "lwgQUXbG1JNs+IzEjQdtbg==";


    cout<<"sEncryptBase64 size:"<<sEncryptedData.size()<<endl;
    cout<<"sIv size:"<<sIv.size()<<endl;

    string sData;

	int ret = oWXBizDataCrypt.DecryptData(sEncryptedData, sIv, sData );
	cout<<"DecryptData ret:"<<ret<<" size:"<<sData.size()<<" data:"<<endl;
	cout<<sData<<endl<<endl<<endl;

    return 0;
}
