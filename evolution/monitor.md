# Evolution monitor (generation 2 done, 47.5 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c1 | 9.23 | 6 | 7.0 | 94 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| prime | 9.18 | 6 | 7.0 | 92 | (Prime) |
| c27 | 9.03 | 2 | 7.0 | 88 | plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02178, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_tau 5.0->3.57124, line_recency 0.6->0.86373, winners3 3->4 |
| c26 | 9.01 | 2 | 7.0 | 88 | soft_bound 1.0->0.9905, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.599, fatigue_gain3 0.0638->0.1116, line_recency 0.6->0.68608 |
| c12 | 8.98 | 4 | 6.8 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.76686, covariance 1.0->0.55228, istdp_rate 50.0->21.75958, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998, order_gain 4.0->3.28215, line_recency 0.6->0.51872 |
| c22 | 8.89 | 2 | 7.0 | 75 | learning_rate 0.04->0.03569, plastic_budget 4.0->2.63165, istdp_rate 50.0->59.06565, encoding_suppression 1.0->0.97265, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776 |
| c10 | 8.80 | 4 | 6.8 | 85 | plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776 |
| c20 | 8.78 | 2 | 6.5 | 94 | plastic_budget 4.0->4.7963, hetero_ltd 1.0->0.9613, istdp_rate 50.0->106.31583, istdp_target 0.02->0.01823, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, fatigue_gain3 0.0638->0.06271 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 4 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 3 | 24 |
| continual | 4 | 24 |
| retention | 6 | 24 |
| order | 1 | 24 |

- A 40% cue brings back on average 77% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): k>a: 1, m>t: 1, w>e: 1, e>w: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 11, cloud>candy: 4, cloud>mouse: 4, light>house: 4, cloud>water: 3, smile>river: 1, apple>dream: 1, storm>tiger: 1

## Picture so far (all generations together)

- Brain tests so far: 72 (28 versions, generation 3).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 12 | 72 | 0 | 6 |
| capacity | 13 | 72 | 0 | 6 |
| efficiency | 4 | 72 | 0 | 6 |
| wordpairs | 11 | 72 | 0 | 6 |
| continual | 20 | 72 | 0 | 6 |
| retention | 22 | 72 | 0 | 6 |
| order | 1 | 72 | 0 | 6 |

- Old memories getting worse (all tests): wiped out -0.010, pushed aside by new memories +0.022 on average; of 20 failed 'keep old memories' tests, 17 were mainly pushed aside (interference), 3 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 465, stored but NOT recalled 111, not stored 0.
- Big-load probe (true word recall %, mean over brains): c1 32 words 73% (n=4); c1 64 words 40% (n=4); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); prime 32 words 80% (n=6); prime 64 words 36% (n=6)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| order_gain | +0.60 | +0.36 | +0.29 |
| inhibition_radius3 | -0.58 | -0.55 | -0.32 |
| consolidation_rate | -0.52 | -0.36 | -0.24 |
| encoding_suppression | +0.46 | +0.34 | +0.37 |
| spread_plastic | +0.45 | +0.31 | +0.22 |
| separation | -0.37 | -0.25 | -0.17 |
| presynaptic_bound | +0.32 | +0.14 | +0.18 |
| channel_winners3 | -0.26 | -0.09 | -0.26 |
| fire_threshold3 | -0.26 | -0.19 | -0.15 |
| fatigue_gain3 | +0.20 | -0.01 | +0.27 |
| plastic_budget | +0.20 | +0.09 | +0.09 |
| mode_tau | +0.20 | +0.13 | -0.12 |
| consolidated_budget | +0.20 | +0.17 | +0.13 |
| hetero_ltd | +0.18 | +0.19 | +0.07 |
| istdp_rate | +0.14 | +0.20 | +0.13 |
| soft_bound | -0.13 | -0.04 | +0.04 |
| line_recency | +0.10 | +0.13 | -0.03 |
| order_tau | -0.06 | -0.14 | +0.02 |
| winners3 | +0.05 | +0.11 | +0.00 |
| assembly_inhibition | +0.05 | -0.04 | +0.10 |
| learning_rate | -0.02 | -0.12 | +0.02 |
| istdp_target | +0.02 | +0.07 | -0.02 |
| covariance | +0.00 | +0.03 | +0.14 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.