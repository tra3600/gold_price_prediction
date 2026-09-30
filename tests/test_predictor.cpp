#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

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

    std::cout << "Tous les tests passent.\n";
    return 0;
}
