# Evolution monitor (generation 6 done, 41.7 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c17 | 10.20 | 8 | 1.4 | 100 | learning_rate 0.02222->0.02412, plastic_budget 5.67831->7.92854, presynaptic_bound 0.89812->0.8741, covariance 0.58435->0.79134, consolidated_budget 0.50842->0.5, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, fire_threshold3 0.02107->0.03121, spread_plastic 0.45398->0.56147, link_bound 0.3->0.12925, silence_gate 0.6->0.48732 |
| c28 | 9.85 | 2 | 1.0 | 100 | learning_rate 0.02222->0.01569, hetero_ltd 0.9733->0.99241, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, order_tau 2.90192->4.08064, mode_tau 13.44333->19.44795, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208 |
| c1 | 9.80 | 10 | 1.2 | 100 | presynaptic_bound 0.89812->0.78512, hetero_ltd 0.9733->1.0, mode_tau 13.44333->13.11295, fatigue_gain3 0.04569->0.07187, silence_gate 0.6->0.29702 |
| c29 | 9.77 | 2 | 1.0 | 100 | learning_rate 0.02222->0.02117, plastic_budget 5.67831->7.40111, presynaptic_bound 0.89812->0.90772, consolidation_rate 0.0056->0.0, consolidated_budget 0.50842->0.50793, assembly_inhibition 0.00321->0.00513, order_gain 6.23347->7.6086, mode_tau 13.44333->21.6521, line_recency 0.48403->0.4, silence_gate 0.6->0.7538 |
| c5 | 9.70 | 14 | 1.1 | 97 | hetero_ltd 0.9733->0.99241, consolidated_budget 0.50842->0.5, mode_tau 13.44333->19.44795, winners3 5->6, spread_plastic 0.45398->0.25208 |
| c30 | 9.69 | 2 | 1.0 | 100 | presynaptic_bound 0.89812->0.78512, hetero_ltd 0.9733->0.95489, mode_tau 13.44333->13.11295, fatigue_gain3 0.04569->0.05135, fire_threshold3 0.02107->0.02527, channel_winners3 2->3, link_bound 0.3->0.28801, silence_gate 0.6->0.29702 |
| c2 | 9.64 | 6 | 1.0 | 100 | consolidation_rate 0.0056->0.0, consolidated_budget 0.50842->0.50793, mode_tau 13.44333->21.6521, line_recency 0.48403->0.4, silence_gate 0.6->0.7538 |
| c31 | 9.56 | 2 | 1.0 | 94 | presynaptic_bound 0.89812->0.98622, hetero_ltd 0.9733->1.0, consolidation_rate 0.0056->0.0, consolidated_budget 0.50842->0.50793, mode_tau 13.44333->21.6521, line_recency 0.48403->0.4, separation 4.94742->5.1622, silence_gate 0.6->0.7538 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| alphabet | 13 | 16 |
| wordpairs | 0 | 16 |

- Word pairs recalled wrongly (cue>recalled: times): smile>river: 1

## Picture so far (all generations together)

- Brain tests so far: 112 (32 versions, generation 7).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| alphabet | 104 | 112 | 13 | 14 |
| wordpairs | 3 | 112 | 0 | 14 |

- Word pairs (8 pairs per test, all tests): stored and recalled 874, stored but NOT recalled 22, not stored 0.

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.76 | +nan | +nan |
| link_bound | -0.45 | +nan | +nan |
| order_tau | +0.36 | +nan | +nan |
| inhibition_radius3 | -0.35 | +nan | +nan |
| separation | -0.32 | +nan | +nan |
| covariance | +0.26 | +nan | +nan |
| channel_winners3 | -0.23 | +nan | +nan |
| mode_tau | +0.21 | +nan | +nan |
| fire_threshold3 | +0.20 | +nan | +nan |
| assembly_inhibition | +0.18 | +nan | +nan |
| order_gain | +0.16 | +nan | +nan |
| fatigue_gain3 | +0.16 | +nan | +nan |
| plastic_budget | +0.16 | +nan | +nan |
| presynaptic_bound | -0.14 | +nan | +nan |
| silence_gate | -0.12 | +nan | +nan |
| winners3 | +0.10 | +nan | +nan |
| hetero_ltd | +0.09 | +nan | +nan |
| line_recency | -0.08 | +nan | +nan |
| spread_plastic | -0.06 | +nan | +nan |
| consolidated_budget | -0.02 | +nan | +nan |
| consolidation_rate | +0.02 | +nan | +nan |
| learning_rate | -0.01 | +nan | +nan |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.