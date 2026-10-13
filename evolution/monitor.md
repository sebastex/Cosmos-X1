# Evolution monitor (generation 15 done, 34.4 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c66 | 10.42 | 2 | 1.5 | 100 | learning_rate 0.02222->0.01, plastic_budget 5.67831->2.50465, presynaptic_bound 0.89812->0.80421, hetero_ltd 0.9733->0.75833, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, assembly_inhibition 0.00321->0.00299, order_tau 2.90192->3.9019, mode_tau 13.44333->19.44795, fire_threshold3 0.02107->0.01753, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.18094, silence_gate 0.6->0.12437 |
| c51 | 10.20 | 10 | 1.4 | 99 | learning_rate 0.02222->0.01569, plastic_budget 5.67831->3.0247, presynaptic_bound 0.89812->0.99365, hetero_ltd 0.9733->0.75833, covariance 0.58435->0.55438, consolidation_rate 0.0056->0.06977, consolidated_budget 0.50842->0.5, order_tau 2.90192->3.95146, mode_tau 13.44333->19.44795, fire_threshold3 0.02107->0.02334, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208, silence_gate 0.6->0.3818 |
| c36 | 10.07 | 10 | 1.3 | 99 | learning_rate 0.02222->0.01569, plastic_budget 5.67831->3.0247, presynaptic_bound 0.89812->0.80421, hetero_ltd 0.9733->0.75833, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, order_tau 2.90192->3.95146, mode_tau 13.44333->19.44795, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208, silence_gate 0.6->0.3818 |
| c60 | 10.03 | 4 | 1.2 | 97 | learning_rate 0.02222->0.01569, plastic_budget 5.67831->2.50465, presynaptic_bound 0.89812->0.80421, hetero_ltd 0.9733->0.75833, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, assembly_inhibition 0.00321->0.00038, order_tau 2.90192->3.95146, mode_tau 13.44333->19.44795, fire_threshold3 0.02107->0.01753, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.18094, silence_gate 0.6->0.37104 |
| c17 | 9.99 | 22 | 1.2 | 100 | learning_rate 0.02222->0.02412, plastic_budget 5.67831->7.92854, presynaptic_bound 0.89812->0.8741, covariance 0.58435->0.79134, consolidated_budget 0.50842->0.5, assembly_inhibition 0.00321->0.00463, order_gain 6.23347->8.0, fire_threshold3 0.02107->0.03121, spread_plastic 0.45398->0.56147, link_bound 0.3->0.12925, silence_gate 0.6->0.48732 |
| c28 | 9.86 | 6 | 1.2 | 100 | learning_rate 0.02222->0.01569, hetero_ltd 0.9733->0.99241, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, order_tau 2.90192->4.08064, mode_tau 13.44333->19.44795, line_recency 0.48403->0.4, winners3 5->6, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208 |
| c64 | 9.77 | 2 | 1.0 | 100 | learning_rate 0.02222->0.02412, plastic_budget 5.67831->7.92854, presynaptic_bound 0.89812->0.71061, hetero_ltd 0.9733->0.51177, covariance 0.58435->0.79134, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, assembly_inhibition 0.00321->0.00226, order_gain 6.23347->8.0, fire_threshold3 0.02107->0.02838, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208, link_bound 0.3->0.12925, silence_gate 0.6->0.3818 |
| c50 | 9.74 | 4 | 1.2 | 100 | learning_rate 0.02222->0.02237, plastic_budget 5.67831->3.0247, presynaptic_bound 0.89812->0.80421, hetero_ltd 0.9733->0.75833, consolidation_rate 0.0056->0.0653, consolidated_budget 0.50842->0.5, assembly_inhibition 0.00321->0.00289, order_tau 2.90192->3.95146, mode_tau 13.44333->19.44795, line_recency 0.48403->0.4, winners3 5->6, channel_winners3 2->3, separation 4.94742->3.18967, spread_plastic 0.45398->0.25208, silence_gate 0.6->0.25294 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| alphabet | 15 | 16 |
| wordpairs | 0 | 16 |


## Picture so far (all generations together)

- Brain tests so far: 256 (68 versions, generation 16).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| alphabet | 229 | 256 | 30 | 32 |
| wordpairs | 4 | 256 | 0 | 32 |

- Word pairs (8 pairs per test, all tests): stored and recalled 2004, stored but NOT recalled 38, not stored 6.

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.33 | +nan | +nan |
| silence_gate | -0.20 | +nan | +nan |
| inhibition_radius3 | -0.20 | +nan | +nan |
| link_bound | -0.16 | +nan | +nan |
| learning_rate | -0.14 | +nan | +nan |
| presynaptic_bound | -0.11 | +nan | +nan |
| hetero_ltd | -0.11 | +nan | +nan |
| order_gain | +0.10 | +nan | +nan |
| mode_tau | +0.10 | +nan | +nan |
| consolidation_rate | +0.10 | +nan | +nan |
| spread_plastic | -0.08 | +nan | +nan |
| plastic_budget | -0.08 | +nan | +nan |
| fatigue_gain3 | +0.08 | +nan | +nan |
| assembly_inhibition | +0.07 | +nan | +nan |
| channel_winners3 | -0.07 | +nan | +nan |
| covariance | +0.06 | +nan | +nan |
| order_tau | +0.05 | +nan | +nan |
| fire_threshold3 | +0.05 | +nan | +nan |
| separation | -0.05 | +nan | +nan |
| winners3 | +0.01 | +nan | +nan |
| consolidated_budget | +0.01 | +nan | +nan |
| line_recency | +0.01 | +nan | +nan |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.