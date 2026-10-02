# Evolution monitor (generation 7 done, 42.0 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c46 | 9.09 | 2 | 7.0 | 88 | learning_rate 0.04->0.05924, plastic_budget 4.0->3.19911, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->4.49026, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.54988, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.5211 |
| c30 | 9.08 | 4 | 7.0 | 88 | istdp_rate 50.0->106.31583, istdp_target 0.02->0.01918, consolidated_budget 0.5->0.7776, order_gain 4.0->4.43802, order_tau 5.0->2.85837, fatigue_gain3 0.0638->0.1116, fire_threshold3 0.0256->0.0267, line_recency 0.6->0.9167 |
| c1 | 9.05 | 14 | 6.9 | 91 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, separation 3.0->0.5762 |
| c36 | 9.03 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.7776, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->27.60532, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c48 | 9.02 | 2 | 7.0 | 82 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.01572, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->1.06565, assembly_inhibition 0.0->0.00132, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->44.6549, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.19974, spread_plastic 0.5->0.1191 |
| c51 | 8.98 | 2 | 7.0 | 88 | learning_rate 0.04->0.05924, plastic_budget 4.0->4.7963, istdp_rate 50.0->106.31583, istdp_target 0.02->0.02106, consolidation_rate 0.0->0.00851, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->3.57124, mode_tau 30.0->15.79689, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.10301 |
| c12 | 8.98 | 4 | 6.8 | 88 | learning_rate 0.04->0.0393, soft_bound 1.0->0.76686, covariance 1.0->0.55228, istdp_rate 50.0->21.75958, consolidation_rate 0.0->0.02789, consolidated_budget 0.5->0.75998, order_gain 4.0->3.28215, line_recency 0.6->0.51872 |
| c31 | 8.98 | 8 | 6.9 | 88 | soft_bound 1.0->0.887, order_gain 4.0->4.7958, order_tau 5.0->3.00249, mode_tau 30.0->29.82312, separation 3.0->0.5762, spread_plastic 0.5->0.23377 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 3 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 3 | 24 |
| continual | 5 | 24 |
| retention | 9 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 57% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): a>q: 1, t>z: 1, e>a: 1, m>q: 1
- Word pairs recalled wrongly (cue>recalled: times): plant>river: 8, smile>candy: 6, cloud>mouse: 5, green>tiger: 4, light>house: 2, cloud>house: 2, smile>mouse: 1, cloud>dream: 1

## Picture so far (all generations together)

- Brain tests so far: 192 (68 versions, generation 8).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 21 | 192 | 0 | 16 |
| capacity | 33 | 192 | 1 | 16 |
| efficiency | 8 | 192 | 0 | 16 |
| wordpairs | 25 | 192 | 1 | 16 |
| continual | 38 | 192 | 2 | 16 |
| retention | 45 | 192 | 2 | 16 |
| order | 1 | 192 | 0 | 16 |

- Old memories getting worse (all tests): wiped out -0.011, pushed aside by new memories +0.014 on average; of 38 failed 'keep old memories' tests, 29 were mainly pushed aside (interference), 9 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 1272, stored but NOT recalled 259, not stored 5.
- Big-load probe (true word recall %, mean over brains): c1 32 words 69% (n=12); c1 64 words 40% (n=12); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 34% (n=2); c30 64 words 6% (n=2); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c44 32 words 84% (n=2); c44 64 words 36% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); c52 32 words 66% (n=2); c52 64 words 23% (n=2); c57 32 words 75% (n=2); c57 64 words 31% (n=2); prime 32 words 73% (n=16); prime 64 words 32% (n=16)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| inhibition_radius3 | -0.46 | -0.33 | -0.29 |
| order_gain | +0.41 | +0.27 | +0.19 |
| encoding_suppression | +0.40 | +0.29 | +0.22 |
| consolidation_rate | -0.39 | -0.24 | -0.18 |
| separation | -0.32 | -0.27 | -0.10 |
| plastic_budget | +0.27 | +0.15 | +0.19 |
| presynaptic_bound | +0.27 | +0.13 | +0.11 |
| hetero_ltd | +0.17 | +0.18 | +0.04 |
| soft_bound | -0.14 | -0.05 | +0.03 |
| channel_winners3 | -0.13 | -0.07 | -0.02 |
| mode_tau | +0.13 | +0.05 | -0.03 |
| learning_rate | -0.12 | -0.09 | -0.10 |
| winners3 | +0.12 | +0.12 | +0.03 |
| spread_plastic | +0.05 | -0.10 | -0.01 |
| covariance | +0.04 | +0.04 | +0.11 |
| istdp_rate | +0.04 | +0.11 | +0.09 |
| fatigue_gain3 | +0.04 | -0.06 | +0.12 |
| fire_threshold3 | -0.04 | -0.05 | -0.03 |
| order_tau | -0.03 | -0.13 | -0.09 |
| consolidated_budget | -0.02 | +0.09 | -0.05 |
| line_recency | -0.02 | +0.09 | +0.04 |
| assembly_inhibition | -0.02 | -0.11 | +0.10 |
| istdp_target | +0.01 | +0.04 | -0.10 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.