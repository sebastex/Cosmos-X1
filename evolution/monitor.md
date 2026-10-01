# Evolution monitor (generation 0 done, 49.5 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c4 | 9.20 | 2 | 7.0 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.67306, covariance 1.0->0.76755, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998 |
| c1 | 9.12 | 2 | 7.0 | 88 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| prime | 9.08 | 2 | 7.0 | 88 | (Prime) |
| c11 | 9.02 | 2 | 7.0 | 82 | plastic_budget 4.0->3.35106, hetero_ltd 1.0->0.60304, covariance 1.0->0.91775, istdp_rate 50.0->46.37326, inhibition_radius3 2->3 |
| c10 | 8.60 | 2 | 6.5 | 88 | plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776 |
| c5 | 8.39 | 2 | 6.5 | 82 | istdp_rate 50.0->56.44993, consolidation_rate 0.0->0.04656, consolidated_budget 0.5->0.55843, order_gain 4.0->1.16383, separation 3.0->4.41858 |
| c2 | 8.38 | 2 | 6.0 | 100 | istdp_rate 50.0->8.12075, istdp_target 0.02->0.01998, assembly_inhibition 0.0->0.00688, mode_tau 30.0->24.75077, separation 3.0->4.23044 |
| c9 | 7.89 | 2 | 6.0 | 88 | learning_rate 0.04->0.05648, consolidated_budget 0.5->0.61213, line_recency 0.6->0.86836, winners3 3->4, spread_plastic 0.5->0.41154 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 4 | 24 |
| capacity | 2 | 24 |
| efficiency | 0 | 24 |
| wordpairs | 0 | 24 |
| continual | 7 | 24 |
| retention | 6 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 73% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): m>a: 2, k>e: 1, w>a: 1, t>w: 1, e>a: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 10, smile>river: 4, light>river: 2, light>house: 2, green>mouse: 1, plant>candy: 1, light>tiger: 1, smile>mouse: 1

## Picture so far (all generations together)

- Brain tests so far: 24 (12 versions, generation 1).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 4 | 24 | 0 | 2 |
| capacity | 2 | 24 | 0 | 2 |
| efficiency | 0 | 24 | 0 | 2 |
| wordpairs | 0 | 24 | 0 | 2 |
| continual | 7 | 24 | 0 | 2 |
| retention | 6 | 24 | 0 | 2 |
| order | 0 | 24 | 0 | 2 |

- Old memories getting worse (all tests): wiped out -0.010, pushed aside by new memories +0.004 on average; of 7 failed 'keep old memories' tests, 6 were mainly pushed aside (interference), 1 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 168, stored but NOT recalled 24, not stored 0.
- Big-load probe (true word recall %, mean over brains): prime 32 words 84% (n=2); prime 64 words 41% (n=2)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| line_recency | -0.40 | -0.12 | -0.74 |
| mode_tau | -0.40 | +0.01 | -0.95 |
| spread_plastic | +0.39 | +0.13 | +0.68 |
| soft_bound | -0.33 | -0.30 | -0.12 |
| covariance | -0.32 | -0.30 | -0.12 |
| separation | -0.27 | -0.25 | +0.07 |
| istdp_rate | +0.26 | +0.11 | +0.46 |
| learning_rate | -0.24 | -0.29 | -0.06 |
| fire_threshold3 | +0.22 | +0.41 | -0.09 |
| istdp_target | +0.22 | +0.20 | -0.09 |
| hetero_ltd | -0.21 | -0.23 | -0.09 |
| consolidation_rate | +0.19 | +0.14 | +0.15 |
| consolidated_budget | +0.14 | -0.01 | +0.22 |
| fatigue_gain3 | -0.11 | -0.20 | +0.09 |
| channel_winners3 | +0.11 | +0.20 | -0.09 |
| winners3 | -0.10 | -0.20 | +0.09 |
| order_gain | +0.06 | +0.07 | -0.07 |
| plastic_budget | -0.06 | -0.13 | +0.01 |
| assembly_inhibition | -0.03 | -0.09 | +0.11 |
| inhibition_radius3 | -0.01 | -0.13 | +0.13 |
| presynaptic_bound | +nan | +nan | +nan |
| encoding_suppression | +nan | +nan | +nan |
| order_tau | +nan | +nan | +nan |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.