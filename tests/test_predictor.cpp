#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <stdexcept>

#include "../DataFetcher.h"
#include "../GoldPricePredictor.h"

static bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }

int main() {
    // Droite parfaite y = 100 + 10 t : la régression doit la retrouver exactement.
    std::vector<double> y;
    for (int i = 0; i < 12; ++i) y.push_back(100 + 10 * i);
    Forecast f = forecastLinear(y, 3);
    assert(near(f.mean[0], 220) && near(f.mean[2], 240));
    assert(near(f.upper[0], f.lower[0])); // bruit nul => intervalle nul

    // Holt suit aussi une tendance parfaite.
    Forecast h = forecastHolt(y, 2);
    assert(near(h.mean[0], 220, 1e-3) && near(h.mean[1], 230, 1e-3));

    // Backtest d'une droite parfaite : erreur nulle.
    assert(near(backtest(y, ModelType::Linear, 3).rmse, 0.0));

    // Erreurs de validation.
    bool threw = false;
    try { forecastLinear({1, 2}, 3); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    // Prévisions par défaut : 12 lignes, mois qui s'enchaînent sur 2025, USD = EUR * taux.
    GoldPricePredictor p;
    p.predictPrices();
    const auto& r = p.predictions();
    assert(r.size() == 12);
    assert(r.front().year == 2025 && r.front().month == 1);
    assert(r.back().year == 2025 && r.back().month == 12);
    for (const auto& row : r) {
        assert(near(row.goldUSD, row.goldEUR * row.usdPerEur));
        assert(row.goldEURLow <= row.goldEUR && row.goldEUR <= row.goldEURHigh);
    }
    // Tendance haussière : prix strictement croissants (l'ancien code dérivait).
    for (size_t i = 1; i < r.size(); ++i) assert(r[i].goldEUR > r[i - 1].goldEUR);
    assert(near(r[0].goldEUR, 1740, 1e-3));

    // Données invalides rejetées.
    threw = false;
    try { p.setData({{2024, 1, 1500, 1.1}}); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);

    // Parsing Stooq + conversion or USD -> EUR.
    std::string csv = "Date,Open,High,Low,Close,Volume\n";
    std::string fxcsv = csv;
    for (int m = 1; m <= 8; ++m) {
        char l1[80], l2[80];
        std::snprintf(l1, sizeof l1, "2023-%02d-30,0,0,0,%d,0\n", m, 2000 + 10 * m);
        std::snprintf(l2, sizeof l2, "2023-%02d-30,0,0,0,1.25,0\n", m);
        csv += l1;
        fxcsv += l2;
    }
    csv += "garbage line\n";
    auto g = parseStooqCSV(csv), x = parseStooqCSV(fxcsv);
    assert(g.size() == 8 && x.size() == 8);
    auto hist = mergeGoldAndFx(g, x, 6);
    assert(hist.size() == 6 && hist.front().month == 3 && hist.back().month == 8);
    assert(near(hist.back().goldEUR, 2080 / 1.25) && near(hist.back().usdPerEur, 1.25));
    threw = false;
    try { mergeGoldAndFx(g, {}, 6); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);
    threw = false;
    try { httpGet("http://x.y/; rm -rf /"); } catch (const std::runtime_error&) { threw = true; }
    assert(threw); // injection shell refusée

    std::cout << "Tous les tests passent.\n";
    return 0;
}
