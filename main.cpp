#include <iostream>
#include "GoldPricePredictor.h"

int main() {
    GoldPricePredictor predictor;
    predictor.predictPrices();
    predictor.printPredictions();
    return 0;
}