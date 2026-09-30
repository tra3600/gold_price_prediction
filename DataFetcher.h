#ifndef DATAFETCHER_H
#define DATAFETCHER_H

#include <map>
#include <string>
#include <vector>

#include "GoldPricePredictor.h"

// Source par défaut : Stooq (CSV publics, sans clé d'API).
// URL finale : <base>?s=<symbole>&i=m  (i=m : barres mensuelles)
extern const char* const kDefaultSourceUrl;

// Télécharge une URL via `curl` (doit être installé). Lève std::runtime_error.
std::string httpGet(const std::string& url);

// Parse un CSV Stooq "Date,Open,High,Low,Close,Volume" -> {année*12+mois-1 : Close}.
std::map<int, double> parseStooqCSV(const std::string& csv);

// Combine or (USD/once) et EURUSD (USD pour 1 EUR) en historique EUR/USD,
// limité aux `months` derniers mois communs. Lève std::runtime_error.
std::vector<MonthlyData> mergeGoldAndFx(const std::map<int, double>& goldUSD,
                                        const std::map<int, double>& eurusd, int months);

// Récupère l'historique en ligne (or XAUUSD + EURUSD). Le mois en cours (incomplet) est exclu.
std::vector<MonthlyData> fetchOnlineHistory(int months, const std::string& baseUrl = kDefaultSourceUrl);

#endif // DATAFETCHER_H
