#include "datalib/Func.h"
#include <stdlib.h>


double Average(NumericSeries prices, int length)
{
	return Summation(prices, length) / min(length, prices.Size());
}

double AverageFC(NumericSeries avgs, NumericSeries prices, int length)
{
	if (prices.Size() <= length)
	{
		return avgs[1] * (prices.Size() - 1) / prices.Size() + prices / prices.Size();
	} 
	else
	{
		return avgs[1] + (prices - prices[length]) / length;;
	}
}

double Summation(NumericSeries prices, int length)
{
	double sum = 0.0;
	for (int i = 0; i < length; ++i)
	{
		sum += prices[i]; 
	}
	return sum;
}

double SummationFC(NumericSeries sums, NumericSeries prices, int length)
{	
	return sums[1] + prices - sums[length];
}
bool DownCross(NumericSeries fastMovAvg, NumericSeries slowMovAvg)
{
	if (fastMovAvg.Size() < 2) return false;
	return fastMovAvg[1] > slowMovAvg[1] && fastMovAvg[0] < slowMovAvg[0];
}

bool UpCross(NumericSeries fastMovAvg, NumericSeries slowMovAvg)
{
	if (fastMovAvg.Size() < 2) return false;
	return fastMovAvg[1] < slowMovAvg[1] && fastMovAvg[0] > slowMovAvg[0];
}

double Highest(NumericSeries prices, int length, int offset)
{
	double val = 0;
	int len = min(length, prices.Size() - offset);
	for (int i = 0; i < len; ++i)
	{
		if (val < prices[i + offset]) 
		{
			val = prices[i + offset];
		}
	}
	return val;
}

double Lowest(NumericSeries prices, int length, int offset)
{
	double val = prices[0];
	int len = min(length, prices.Size() - offset);
	for (int i = 0; i < len; ++i)
	{
		if (val > prices[i + offset]) 
		{
			val = prices[i + offset];
		}
	}
	return val;
}

double RelativeStrength(NumericSeries prices, int length)
{
	if (prices.Size() <= length) return 50.0;
	double up_sum = 0.0000001; // UP ���̺�
	double down_sum = 0.0000001; // DOWN���̺�
	for (int i = 0; i < length; ++i) {
		if (prices[i] > prices[i + 1])
			up_sum += prices[i] - prices[i + 1];
		else 
			down_sum += prices[i + 1] - prices[i];
	}
	return up_sum / (up_sum + down_sum) * 100;
}

double RelativeStrengthFC(NumericSeries rsis, NumericSeries prices, int length)
{
	if (prices.Size() <= length) return 50.0;

	double up_sum = 0.0000001; // UP ���̺�
	double down_sum = 0.0000001; // DOWN ���̺�

	if (prices.Size() == length + 1)
	{
		return RelativeStrength(prices, length);
	}
	else
	{
		if (prices[0] > prices[1]) up_sum += prices[0] - prices[1]; // ���Ͻ���� UP ���̣����У�
		if (prices[length] > prices[length+1]) up_sum -= prices[length] - prices[length+1]; // ��ȥ1��lengthǰ�����UP���̣����У�
		if (prices[0] < prices[1]) down_sum += prices[1] - prices[0]; // ���Ͻ���� DOWN ���̣����У�
		if (prices[length] < prices[length+1]) down_sum -= prices[length+1] - prices[length]; // ��ȥ1��lengthǰ�����DOWN���̣����У�
	}

	double rsi = up_sum / (up_sum + down_sum) * 100;
	return rsi;
}


double StandardDev(NumericSeries avgs, NumericSeries prices, int length)
{
	double dev = 0.0;
	for (int i = 0; i < min(length, prices.Size()); ++i) {
		dev += pow(avgs[i] - prices[i], 2);
	}
	return sqrt(dev / min(length, prices.Size()));
}

double XAverage(NumericSeries xAvgs, NumericSeries prices, int length)
{
	double smoothFactor = 2.0 / (length + 1);
	double xAvg = xAvgs[1]  + (prices - xAvgs[1]) * smoothFactor;
	return xAvg;
}