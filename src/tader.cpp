#include <stdio.h>
#include <windows.h>
#include <iostream>
#include "..\libs\TraderApi\apitraderapi\QdpFtdcTraderApi.h"
using  namespace std;
// 报单录入操作是否完成的标志
// Create a manual reset event with no signal
HANDLE g_hEvent = CreateEvent(NULL, true, false, NULL);
// 经纪公司代码
TQdpFtdcBrokerIDType g_chBrokerID;
// 交易用户代码
TQdpFtdcUserIDType g_chUserID;
// 用户本地最大报单号
int g_UserOrderLocalID;
class CSimpleHandler : public CQdpFtdcTraderSpi
{
public:
	// 构造函数，需要一个有效的指向CQdpFtdcMduserApi实例的指针
	CSimpleHandler(CQdpFtdcTraderApi *pUserApi) : m_pUserApi(pUserApi) {}
	~CSimpleHandler() {}
	// 当客户端与QDP建立起通信连接，客户端需要进行登录
	virtual void OnFrontConnected()
	{
		CQdpFtdcReqUserLoginField reqUserLogin;
		// get BrokerID
		printf("BrokerID:");
		scanf("%s", &g_chBrokerID);
		strcpy(reqUserLogin.BrokerID, g_chBrokerID);
		// get userid
		printf("userid:");
		scanf("%s", &g_chUserID);
		strcpy(reqUserLogin.UserID, g_chUserID);
		// get password
		printf("password:");
		scanf("%s", &reqUserLogin.Password);
		// 发出登陆请求
		m_pUserApi->ReqUserLogin(&reqUserLogin, 0);
	}
	// 当客户端与QDP通信连接断开时，该方法被调用
	virtual void OnFrontDisconnected(int nReason)
	{
		// 当发生这个情况后，API会自动重新连接，客户端可不做处理
		printf("OnFrontDisconnected.\n");
	}
	// 当客户端发出登录请求之后，该方法会被调用，通知客户端登录是否成功
	virtual void OnRspUserLogin(
		CQdpFtdcRspUserLoginField *pRspUserLogin,
		CQdpFtdcRspInfoField *pRspInfo,
		int nRequestID,
		bool bIsLast)
	{
		printf("OnRspUserLogin:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n",
			   pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
		if (pRspInfo->ErrorID != 0)
		{
			// 登录失败，客户端需进行错误处理
			printf("Failed to login, errorcode=%d errormsg=%s,requestid=%d chain=%d", pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast);
			exit(-1);
		}
		// 用户最大本地报单号
		g_UserOrderLocalID = atoi(pRspUserLogin->MaxOrderLocalID) + 1;
		// 登录成功,发出报单录入请求
		CQdpFtdcInputOrderField ord;
		memset(&ord, 0, sizeof(ord));
		// 经纪公司代码
		strcpy(ord.BrokerID, g_chBrokerID);
		// 合约代码
		strcpy(ord.InstrumentID, "IF1306");
		// 投资者代码
		strcpy(ord.InvestorID, "000101");
		// 用户代码
		strcpy(ord.UserID, g_chUserID);
		// 买卖方向
		ord.Direction = '2';
		// 开平标志
		ord.OffsetFlag = '0';
		// 投机套保标志
		ord.HedgeFlag = '1';
		// 价格
		ord.LimitPrice = 50000;
		// 数量
		ord.VolumeTotalOriginal = 10;
		// 有效期类型
		strcpy(ord.TimeCondition, "1");
		// 自动挂起标志
		ord.IsAutoSuspend = 0;
		// 交易所
		strcpy(ord.ExchangeID, "CFFEX");
		// 本地报单号
		sprintf(ord.UserOrderLocalID, "%012d", g_UserOrderLocalID++);
		m_pUserApi->ReqOrderInsert(&ord, 1);
	}
	// 报单录入应答
	virtual void OnRspOrderInsert(
		CQdpFtdcInputOrderField *pInputOrder,
		CQdpFtdcRspInfoField *pRspInfo,
		int nRequestID,
		bool bIsLast)
	{
		// 输出报单录入结果
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n",
			   pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		// 通知报单录入完成
		SetEvent(g_hEvent);
	};
	/// 报单回报
	virtual void OnRtnOrder(CQdpFtdcOrderField *pOrder)
	{
		printf("OnRtnOrder:\n");
		printf("OrderSysID=[%s]\n", pOrder->OrderSysID);
	}
	// 针对用户请求的出错通知
	virtual void OnRspError(
		CQdpFtdcRspInfoField *pRspInfo,
		int nRequestID,
		bool bIsLast)
	{
		printf("OnRspError:\n");
		printf("ErrorCode=[%d], ErrorMsg=[%s]\n",
			   pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		printf("RequestID=[%d], Chain=[%d]\n", nRequestID, bIsLast);
		// 客户端需进行错误处理
		{
			// 客户端的错误处理
		}
	}

private:
	// 指向CQdpFtdcTraderApi实例的指针
	CQdpFtdcTraderApi *m_pUserApi;
};
int main()
{
	cout << 111 << endl;
	// 产生一个CQdpFtdcTraderApi实例
	CQdpFtdcTraderApi *pUserApi =
		CQdpFtdcTraderApi::CreateFtdcTraderApi();
	// 产生一个事件处理的实例
	CSimpleHandler sh(pUserApi);
	// 注册一事件处理的实例
	pUserApi->RegisterSpi(&sh);
	// 订阅私有流
	// QDP_TERT_RESTART:从本交易日开始重传
	// QDP_TERT_RESUME:从上次收到的续传
	// QDP_TERT_QUICK:只传送登录后私有流的内容
	pUserApi->SubscribePrivateTopic(QDP_TERT_RESUME);
	// 订阅公共流
	// QDP_TERT_RESTART:从本交易日开始重传
	// QDP_TERT_RESUME:从上次收到的续传
	// QDP_TERT_QUICK:只传送登录后公共流的内容
	pUserApi->SubscribePublicTopic(QDP_TERT_RESUME);
	// 设置量投科技服务的地址，可以注册多个地址备用
	pUserApi->RegisterFront("tcp://172.28.21.133:15555");
	// 使客户端开始与后台服务建立连接
	pUserApi->Init();
	// 客户端等待报单操作完成
	cout << 123 << endl;
	WaitForSingleObject(g_hEvent, INFINITE);
	// 释放API实例
	pUserApi->Release();
	
	return 0;
}
