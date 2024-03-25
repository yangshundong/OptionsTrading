#include <stdio.h>
#include <string.h>
#include <float.h>
#include "..\libs\QdFtdcMdApi\QdFtdcMdApi.h"

class CSimpleHandler : public CQdFtdcMduserSpi {
public:
	CSimpleHandler(CQdFtdcMduserApi *pUserApi) : m_pUserApi(pUserApi) {}
	~CSimpleHandler() {}
	void OnFrontConnected() 
	{
		CQdFtdcReqUserLoginField reqUserLogin;
		strcpy(reqUserLogin.TradingDay, m_pUserApi->GetTradingDay());
		strcpy(reqUserLogin.BrokerID, "0001");
		strcpy(reqUserLogin.UserID, "t002");
		strcpy(reqUserLogin.Password, "111111");		
		m_pUserApi->ReqUserLogin(&reqUserLogin, 0);
	}
	void OnFrontDisconnected()
	{
		printf("OnFrontDisconnected.\n");
	}
	void OnRspUserLogin(CQdFtdcRspUserLoginField *pRspUserLogin, CQdFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspUserLogin:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
		if (pRspInfo->ErrorID != 0) 
		{
			printf("Failed to login, errorcode=%d errormsg=%s requestid=%d chain=%d", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			return;
		}
		char * contracts[3]={"","",""};
		contracts[0]="IF1503";
		contracts[1]="IF1504";
		contracts[2]="IF1506";
		m_pUserApi->SubMarketData(contracts, 3);

		char * uncontracts[2]={"",""};
		uncontracts[0]="IF1509";
		uncontracts[1]="IH1503";
		m_pUserApi->UnSubMarketData(uncontracts, 2);
	}
	void OnRtnDepthMarketData(CQdFtdcDepthMarketDataField *pMarketData)
	{
		printf("%s,%s,%d,",pMarketData->InstrumentID,pMarketData->UpdateTime,pMarketData->UpdateMillisec);
		if (pMarketData->AskPrice1==DBL_MAX)
			printf("%s,","");
		else
			printf("%f,",pMarketData->AskPrice1);
		
		if (pMarketData->BidPrice1==DBL_MAX)
			printf("%s \n","");
		else
			printf("%f \n",pMarketData->BidPrice1);
	}
	
	void OnRspError(CQdFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("OnRspError:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
	}
	
	void OnRspSubMarketData(CQdFtdcSpecificInstrumentField *pSpecificInstrument, CQdFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("Sub %s \n",pSpecificInstrument->InstrumentID);
	}
	
	void OnRspUnSubMarketData(CQdFtdcSpecificInstrumentField *pSpecificInstrument, CQdFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		printf("UnSub%s \n",pSpecificInstrument->InstrumentID);
	}

private:
	CQdFtdcMduserApi *m_pUserApi;
};

int main()
{
	CQdFtdcMduserApi* pUserApi = CQdFtdcMduserApi::CreateFtdcMduserApi();
	CSimpleHandler sh(pUserApi);
	pUserApi->RegisterSpi(&sh);
	pUserApi->RegisterFront("tcp://192.168.1.100:7220");
	pUserApi->Init();
	pUserApi->Release();
	return 0;
}