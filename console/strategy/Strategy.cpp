
#include <iostream>
#include "strategy/Strategy.h"
//#include "dataserver/SubBarsApi.h"
#include "common/Directory.h"
#include "common/XmlConfig.h"
#include "common/AppLog.h"
//#include "dataserver/RuntimeDataApiManager.h"
#include "datalib/SymbolInfoSet.h"
#include "datalib/SimpleMath.h"

Strategy::Strategy()
{
	strncpy(user_tag_, "002907", sizeof(user_tag_));

	trade_engine_ = TradeEngine::Instance();
	trade_engine_->SetSpi(this, this);	

	//data_engine_ = DataEngine::Instance();
	//data_engine_->SetSpi(this);
	client_ = new Client(this);
	
}

Strategy::~Strategy()
{
	delete client_;
}

bool Strategy::Init(string &err){

	/*
	// strategy param
	XmlConfig config(Global::Instance()->GetConfigDir() + "config.xml");
	if (!config.Load()) 
	{
		err = "load config error";
		return false;
	}
	XmlNode node = config.FindNode("Strategy_MA");		
	std::string code = node.GetValue("code");
	symbol_ =  Symbol(PRODUCT_FUTURE, EXCHANGE_DCE, code.c_str());
	std::string period = node.GetValue("kline_period");
	s_ma_.Period = atoi(node.GetValue("short_ma").c_str());
	l_ma_.Period = atoi(node.GetValue("long_ma").c_str());
	state_ = 0;
	int jump_nums = atoi(node.GetValue("jump_nums").c_str());
	price_offset_ = SymbolInfoSet::Instance()->GetPriceTick(symbol_) * jump_nums;
	submit_hands_ = atoi(node.GetValue("submit_hands").c_str());
	cancel_interval_ = node.GetValue("cancel_interval");
	target_profit_value_ = atof(node.GetValue("target_profit_value").c_str());
	*/
	
	symbol_ =  Symbol(PRODUCT_FUTURE, EXCHANGE_DCE, "a2009");
	timer_ = new TimerApi(10000, this);
	//timer_->Start(30000);
	
	client_->Init();
	client_->SubData(symbol_);
	
	if (!trade_engine_->Init(err)) {
		APP_LOG(LOG_LEVEL_ERROR) << err;
		return false;
	}
	return true;
}

void Strategy::Denit() {
	delete timer_;
	client_->Denit();
	trade_engine_->Denit();
}
/*
// OnTick
void Strategy::OnUpdateKline(const Bars *_bars, bool is_new){
	Bars *bars = (Bars*)_bars;

	//cout << "OnTick: " << bars->symbol.instrument << "  "  << bars->DimCount << "  " << bars->UpdateTime.Str() << "  " << bars->LastPrice << endl;

	double pre_price = trade_engine_->GetPosiPrePrice(bars->symbol);
	trade_engine_->SetPosiLastPrice(bars->symbol, bars->LastPrice);

	Locker lock(&pos_mutex_);
	for (int i=0; i < positions_.size(); ++i){
		if (positions_[i].symbol == bars->symbol){
			if (PriceUnEqual(pre_price, bars->LastPrice)) {
				PriceType pre_profit = float_profit_[i];
				if (PriceUnEqual(pre_profit, 0.)) {
					float_profit_[i] = trade_engine_->CalcFloatProfit(
						positions_[i].symbol, positions_[i].direction,
						positions_[i].open_price, bars->LastPrice, positions_[i].open_volume);
					PriceType diff_profit = float_profit_[i] - pre_profit;
					trade_engine_->UpdateAccountProfit(diff_profit);		
				}
				else {
					trade_engine_->UpdateAccountProfit(float_profit_[i], true);	
				}
			}
		}
	}
	
	std::vector<OrderData> orders;
	trade_engine_->GetValidOrder(orders);
	for (int i=0; i < orders.size(); ++i)
	{
		DateTime time = orders[i].submit_time;	
		int num = atoi(cancel_interval_.substr(0, cancel_interval_.size() - 1).c_str());
		if (cancel_interval_[1]=='m')
			time.AddMin(num);
		else if (cancel_interval_[1]=='s')
			time.AddSec(num);
		if (time < bars->UpdateTime) //??????????????????1???????????????
		{
			trade_engine_->CancelOrder(orders[i]);
			OrderParamData param;
			param.symbol = orders[i].symbol;
			param.limit_price = orders[i].direction == LONG_DIRECTION ? bars->AskPrice[0] + price_offset_ : bars->BidPrice[0] - price_offset_;
			param.volume = orders[i].total_volume - orders[i].trade_volume;
			param.order_price_flag = LIMIT_PRICE_ORDER;
			strncpy(param.user_tag, user_tag_, sizeof(UserStrategyIdType));
			param.direction = orders[i].direction;
			param.open_close_flag = orders[i].open_close_flag;
			param.hedge_flag = HF_SPECULATION;
			trade_engine_->SubmitOrder(param);
		}
	}

}
// OnBar
void Strategy::OnKlineFinish(const Bars *_bars){
	Bars *bars = (Bars*)_bars;

	s_ma_.Update(bars);
	l_ma_.Update(bars);
	cout << "s_ma_.Value[1]:"<<s_ma_.Value[1]<<" l_ma_.Value[1]:"<<l_ma_.Value[1]
	<<" s_ma_.Value[0]"<<s_ma_.Value[0]<<" l_ma_.Value[0]"<<l_ma_.Value[0]<<endl;
	switch (state_)
	{
	case 0:
		if (s_ma_.Value[1] < l_ma_.Value[1] && s_ma_.Value[0] > l_ma_.Value[0])
		{
			state_ = 1;
			trade_engine_->Buy(bars->symbol, bars->AskPrice[0] + price_offset_, submit_hands_);
			cout << "buy open\n";
		}
		else if (s_ma_.Value[1] > l_ma_.Value[1] && s_ma_.Value[0] < l_ma_.Value[0])
		{
			state_ = -1;
			trade_engine_->SellShort(bars->symbol, bars->BidPrice[0] - price_offset_, submit_hands_);
			cout << "sell open\n";
		}
		break;
	case 1:
		{
			if (s_ma_.Value[1] > l_ma_.Value[1] && s_ma_.Value[0] < l_ma_.Value[0])
			{
				PositionData long_pos;
				trade_engine_->GetLongPositionBySymbol(long_pos, bars->symbol);

				state_ = 0;
				trade_engine_->Sell(bars->symbol, bars->BidPrice[0] - price_offset_, long_pos.enable_today_volume);
				cout << "sell close\n";
			}
		}
		break;

	case -1:
		{
			if (s_ma_.Value[1] < l_ma_.Value[1] && s_ma_.Value[0] > l_ma_.Value[0])
			{
				PositionData short_pos;
				trade_engine_->GetShortPositionBySymbol(short_pos, bars->symbol);

				state_ = 0;
				trade_engine_->BuyCover(bars->symbol, bars->AskPrice[0] + price_offset_, short_pos.enable_today_volume);
				cout << "buy close\n";
			}
		}
	}

	trade_engine_->QryCtpAccount();
	cout << "OnBar:  " << bars->End[0].Str() << "  O:" << bars->Open[0] << "  H:" << bars->High[0] << "  L:" << bars->Low[0]
	<< "  C:" <<bars->Close[0] << "  V:" << bars->Volume[0] << endl;
}

// Init: OnTick & OnBar
void Strategy::OnInitKline(const Bars *_bars){
	Bars *bars = (Bars*)_bars;
	// OnTick
	s_ma_.Update(bars);
	l_ma_.Update(bars);

	trade_engine_->SetPosiLastPrice(bars->symbol, bars->LastPrice);

	// OnBar	



	cout << "Init OnTick: " << bars->symbol.instrument << "  "  << bars->DimCount << "  " << bars->UpdateTime.Str() << "  " << bars->LastPrice << endl;
	cout << "Init OnBar:  " << bars->End[0].Str() << "  O:" << bars->Open[0] << "  H:" << bars->High[0] << "  L:" << bars->Low[0]
	<< "  C:" <<bars->Close[0] << "  V:" << bars->Volume[0] << endl;
}
*/

void Strategy::UpdateAccount(){
	AccountData account;
	trade_engine_->GetAccount(account);
	cout << "???????????????\n"
		<< "????????????"<<account.static_balance<<" ?????????"<<account.close_profit<<" ?????????"<<account.commision
		<< " ????????????"<<account.asset_balance << " ???????????????"<<account.margin_balance 
		<< " ????????????"<<account.frozen_balance << " ????????????"<<account.enable_balance<< endl;

	// ????????????????????????????????????????????????????????? ?????????????????????????????????????????????.
	if (account.asset_balance > target_profit_value_){
		PositionData long_pos, short_pos;
		double last_price = trade_engine_->GetPosiPrePrice(symbol_);
		if (trade_engine_->GetLongPositionBySymbol(long_pos, symbol_)){
			trade_engine_->Sell(symbol_, last_price-price_offset_, long_pos.enable_today_volume+long_pos.enable_yestd_volume);
		}
		if (trade_engine_->GetShortPositionBySymbol(short_pos, symbol_)){
			trade_engine_->BuyCover(symbol_, last_price+price_offset_, short_pos.enable_today_volume+short_pos.enable_yestd_volume);
		}
	}
}

std::string OpenCloseStr(OpenCloseFlag flag)
{
	switch (flag)
	{
	case OPEN_ORDER:
		return ("??????");
	case CLOSE_ORDER:
		return ("??????");
	case CLOSE_TODAY_ORDER:
		return ("??????");
	case CLOSE_YESTERDAY_ORDER:
		return ("??????");
	default:
		return "";
	}
}

std::string OrderStatusStr(OrderStatus status)
{
	switch(status)
	{
	case '0': return "??????";
	case '1': return "??????";
	case '2': return "?????????";
	case '3': return "????????????";
	case '4': return "????????????";
	case '5': return "??????";
	case '6': return "?????????";
	case '7': return "????????????";
	case '8': return "?????????";
	default: return "????????????"; 
	}
}

void Strategy::UpdateValidOrders(){
	vector<OrderData> validOrders;
	trade_engine_->GetValidOrder(validOrders);
	APP_LOG(LOG_LEVEL_INFO) << "??????????????????";
	for (int i=0;i<validOrders.size();++i){
		APP_LOG(LOG_LEVEL_INFO)
			<< ("????????????:") << validOrders[i].order_id
			<< (" ??????:") << validOrders[i].symbol.instrument
			<< (" ??????:") << (validOrders[i].direction == LONG_DIRECTION ? "??????" : "??????")
			<< (" ??????:") << OpenCloseStr(validOrders[i].open_close_flag)
			<< (" ????????????:") << validOrders[i].limit_price
			<< (" ????????????:") << validOrders[i].total_volume
			<< (" ????????????:") << validOrders[i].trade_volume
			<< (" ????????????:") << validOrders[i].submit_time.time.Str()
			<< (" ??????:") << OrderStatusStr(validOrders[i].status)
			<< (" ????????????:") << validOrders[i].status_msg;
	}
}
void Strategy::UpdateTrades(){
	vector<TradeData> trades;
	trade_engine_->GetAllTrade(trades);
	APP_LOG(LOG_LEVEL_INFO) <<"??????????????????";
	for (int i=0;i<trades.size();++i){
		APP_LOG(LOG_LEVEL_INFO)
			<< ("??????") << trades[i].symbol.instrument
			<< (" ????????????:") << trades[i].order_id
			<< (" ????????????:") << trades[i].trade_id
			<< (" ??????:") << (trades[i].direction == LONG_DIRECTION ? "??????" : "??????")
			<< (" ??????:") << OpenCloseStr(trades[i].open_close_flag)
			<< (" ?????????:") << trades[i].trade_price
			<< (" ??????:") << trades[i].trade_volume
			<< (" ????????????:") << trades[i].trade_time.time.Str();
	}
}
void Strategy::UpdatePositions(){
	double all_profit = 0.;//???????????????.
	pos_mutex_.Lock();
	positions_.clear();
	trade_engine_->GetAllPosition(positions_);
	for (std::vector<PositionData>::const_iterator iter = positions_.begin(); iter != positions_.end(); ++iter) {
		float_profit_.push_back(iter->position_profit);
		all_profit += iter->position_profit;
	}
	pos_mutex_.Unlock();
	trade_engine_->UpdateAccountProfit(all_profit);

	vector<PositionData> positions;
	trade_engine_->GetAllPosition(positions);
	//APP_LOG(LOG_LEVEL_INFO) <<"???????????????";
	for (int i=0;i<positions.size();++i){
		APP_LOG(LOG_LEVEL_INFO)
			<< ("\t") << positions[i].symbol.instrument
			<< ("\t") << (positions[i].direction == LONG_DIRECTION ? "long" : "short")
			<< ("\t") << positions[i].open_volume << " hand"
			//<< (" ??????:") << positions[i].yestd_volume
			//<< (" ??????:") << positions[i].enable_today_volume + positions[i].enable_yestd_volume
			<< ("\topen_price:") << positions[i].open_price;
	}
	if (positions.size()==0){
		APP_LOG(LOG_LEVEL_INFO) << "0 hand";
	}
}
// OnOrder OnTrade
void Strategy::OnTradeEvent(TradeEventData& event){
	if (event.type == TradeEventData::ACCOUNT_EVENT) {
		//UpdateAccount();
		return;
	}
	else if (event.type == TradeEventData::ORDER_EVENT)
	{
		//UpdateValidOrders();
	}
	else if (event.type == TradeEventData::TRADE_EVENT)
	{
		//UpdateTrades();
		UpdatePositions();
	}
	else if (event.type == TradeEventData::POSITION_EVENT)
	{
		UpdatePositions();
	}

	//UpdateAccount();
}
// OnError
void Strategy::OnTradeError(const string &err){
	cout << err << endl;
}

// ????????? ?????? ?????? ; 
void Strategy::OnData(Bars *bars, bool is_kline_up){
	last_price_ = bars->tick.last_price;
	if (strlen(bars->tick.symbol.instrument) == 0) last_price_ = bars->klines[0].close;
	trade_engine_->SetPosiLastPrice(bars->tick.symbol, last_price_);

	if (last_price_ < 0 || last_price_ > 100000000) return;
	if (bars->klines.Size() < 5) return;
	double max = DoubleMax(3, bars->klines[1].high, bars->klines[2].high, bars->klines[3].high);
	double min = DoubleMin(3, bars->klines[1].low, bars->klines[2].low, bars->klines[3].low);
	PositionData long_pos, short_pos;
	trade_engine_->GetLongPositionBySymbol(long_pos, symbol_);
	trade_engine_->GetShortPositionBySymbol(short_pos, symbol_);
	static int state_ = 0;
	switch(state_) {
	case 0:
		{
			if (last_price_ > max) {
				if (short_pos.today_volume > 0)
					trade_engine_->BuyCover(symbol_, last_price_+10, short_pos.today_volume);
				trade_engine_->Buy(symbol_, last_price_+10, 1);
				state_ = 1;
			} else if (last_price_ < min) {
				if (long_pos.today_volume > 0)
					trade_engine_->Sell(symbol_, last_price_-10, long_pos.today_volume);
				trade_engine_->SellShort(symbol_, last_price_-10, 1);
				state_ = -1;
			}
		}
		break;
	case 1:
		{
			if (last_price_ < min) {
				trade_engine_->Sell(symbol_, last_price_-10, long_pos.today_volume);
				state_ = 0;
			}
		}
		break;
	case -1:
		{
			if (last_price_ > max) {
				trade_engine_->BuyCover(symbol_, last_price_+10, short_pos.today_volume);
				state_ = 0;
			}
		}
		break;
	}


	/*Locker lock(&pos_mutex_);
	double pre_price = trade_engine_->GetPosiPrePrice(bars->tick.symbol);
	for (int i=0; i < positions_.size(); ++i){
		if (positions_[i].symbol == bars->tick.symbol){
			if (PriceUnEqual(pre_price, bars->tick.last_price)) {
				PriceType pre_profit = float_profit_[i];
				if (PriceUnEqual(pre_profit, 0.)) {
					float_profit_[i] = trade_engine_->CalcFloatProfit(
						positions_[i].symbol, positions_[i].direction,
						positions_[i].open_price, bars->tick.last_price, positions_[i].open_volume);
					PriceType diff_profit = float_profit_[i] - pre_profit;
					trade_engine_->UpdateAccountProfit(diff_profit);		
				}
				else {
					trade_engine_->UpdateAccountProfit(float_profit_[i], true);	
				}
			}
		}
	}*/

	// print 1m klines
	if (is_kline_up) {
		static int n = 0;
		if (bars->klines.Size() > n){
			if (bars->klines.Size()-n ==1){
				APP_LOG(LOG_LEVEL_INFO) << bars->klines[0].Str();
			} else {
				for (int i=bars->klines.Size()-1; i>=0;--i){
					APP_LOG(LOG_LEVEL_INFO) << bars->klines[i].Str();
				}
			}

			n = bars->klines.Size();
		}
	}
}
void Strategy::OnError(const string &err){

}
void Strategy::OnTimer() {
	if (last_price_ < 0 || last_price_ > 100000000) return;
	PositionData long_pos, short_pos;
	trade_engine_->GetLongPositionBySymbol(long_pos, symbol_);
	trade_engine_->GetShortPositionBySymbol(short_pos, symbol_);
	static int state_ = 0;
	switch (state_)
	{
	case 0: // init
		if (long_pos.today_volume == 0) {
			state_ = 1;
			trade_engine_->Buy(symbol_, last_price_+10, 1);
		} else {
			state_ = 0;
			trade_engine_->Sell(symbol_, last_price_ - 10, long_pos.enable_today_volume);
		}
		
		break;
	case 1:
		if (long_pos.enable_today_volume > 0){
			state_ = 0;
			trade_engine_->Sell(symbol_, last_price_ - 10, long_pos.enable_today_volume);
		}
		
		break;
	}

	//trade_engine_->QryCtpAccount();
}
