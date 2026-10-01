# Evolution monitor (generation 3 done, 46.3 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c30 | 9.22 | 2 | 7.0 | 94 | istdp_rate 50.0->106.31583, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.7776, order_gain 4.0->4.43802, order_tau 5.0->2.85837, fatigue_gain3 0.0638->0.1116, fire_threshold3 0.0256->0.0267, line_recency 0.6->0.9167 |
| c1 | 9.15 | 8 | 7.0 | 89 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| c31 | 9.13 | 2 | 7.0 | 88 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, order_tau 5.0->3.00249, mode_tau 30.0->29.82312, separation 3.0->0.5762, spread_plastic 0.5->0.23377 |
| c12 | 8.98 | 4 | 6.8 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.76686, covariance 1.0->0.55228, istdp_rate 50.0->21.75958, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998, order_gain 4.0->3.28215, line_recency 0.6->0.51872 |
| c22 | 8.89 | 2 | 7.0 | 75 | learning_rate 0.04->0.03569, plastic_budget 4.0->2.63165, istdp_rate 50.0->59.06565, encoding_suppression 1.0->0.97265, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776 |
| c28 | 8.82 | 2 | 7.0 | 75 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, line_recency 0.6->0.64844, winners3 3->4 |
| prime | 8.81 | 8 | 6.8 | 86 | (Prime) |
| c10 | 8.80 | 4 | 6.8 | 85 | plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 6 | 24 |
| efficiency | 0 | 24 |
| wordpairs | 2 | 24 |
| continual | 4 | 24 |
| retention | 3 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 71% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): w>a: 2, m>t: 1, q>e: 1
- Word pairs recalled wrongly (cue>recalled: times): cloud>candy: 13, smile>candy: 9, green>dream: 2, cloud>water: 2, smile>river: 2, smile>mouse: 1, storm>dream: 1, apple>house: 1

## Picture so far (all generations together)

- Brain tests so far: 96 (36 versions, generation 4).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 14 | 96 | 0 | 8 |
| capacity | 19 | 96 | 0 | 8 |
| efficiency | 4 | 96 | 0 | 8 |
| wordpairs | 13 | 96 | 1 | 8 |
| continual | 24 | 96 | 1 | 8 |
| retention | 25 | 96 | 0 | 8 |
| order | 1 | 96 | 0 | 8 |

- Old memories getting worse (all tests): wiped out -0.015, pushed aside by new memories +0.017 on average; of 24 failed 'keep old memories' tests, 21 were mainly pushed aside (interference), 3 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 625, stored but NOT recalled 141, not stored 2.
- Big-load probe (true word recall %, mean over brains): c1 32 words 69% (n=6); c1 64 words 40% (n=6); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); prime 32 words 75% (n=8); prime 64 words 33% (n=8)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| order_gain | +0.58 | +0.33 | +0.24 |
| inhibition_radius3 | -0.52 | -0.44 | -0.30 |
| consolidation_rate | -0.52 | -0.32 | -0.25 |
| encoding_suppression | +0.45 | +0.31 | +0.31 |
| separation | -0.36 | -0.26 | -0.15 |
| presynaptic_bound | +0.33 | +0.16 | +0.15 |
| fire_threshold3 | -0.25 | -0.18 | -0.12 |
| channel_winners3 | -0.23 | -0.04 | -0.15 |
| fatigue_gain3 | +0.20 | -0.01 | +0.17 |
| hetero_ltd | +0.19 | +0.20 | +0.05 |
| mode_tau | +0.19 | +0.11 | -0.11 |
| spread_plastic | +0.18 | +0.01 | +0.24 |
| istdp_rate | +0.18 | +0.24 | +0.16 |
| consolidated_budget | +0.18 | +0.23 | -0.01 |
| order_tau | -0.17 | -0.25 | -0.05 |
| plastic_budget | +0.15 | +0.00 | +0.09 |
| line_recency | +0.13 | +0.16 | +0.01 |
| soft_bound | -0.10 | -0.02 | +0.09 |
| winners3 | +0.07 | +0.17 | -0.02 |
| covariance | +0.04 | +0.00 | +0.21 |
| learning_rate | +0.03 | -0.04 | +0.09 |
| assembly_inhibition | +0.03 | -0.05 | +0.09 |
| istdp_target | +0.01 | +0.10 | -0.09 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.