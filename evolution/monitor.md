# Evolution monitor (generation 18 done, 29.8 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c134 | 9.35 | 6 | 7.0 | 96 | learning_rate 0.04->0.04627, soft_bound 1.0->0.887, presynaptic_bound 1.0->0.86613, istdp_rate 50.0->20.87958, order_gain 4.0->5.52885, mode_tau 30.0->29.52901, line_recency 0.6->0.54086, winners3 3->4, channel_winners3 4->3, separation 3.0->0.5762 |
| c154 | 9.33 | 2 | 7.0 | 100 | plastic_budget 4.0->5.67831, soft_bound 1.0->0.75042, hetero_ltd 1.0->0.84505, istdp_rate 50.0->141.57696, istdp_target 0.02->0.02085, consolidation_rate 0.0->0.0056, order_gain 4.0->4.7958, order_tau 5.0->4.17032, mode_tau 30.0->17.38749, fatigue_gain3 0.0638->0.05696, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.55852, winners3 3->5, channel_winners3 4->3, separation 3.0->4.07163 |
| c146 | 9.29 | 4 | 7.0 | 100 | learning_rate 0.04->0.04627, presynaptic_bound 1.0->0.86613, istdp_rate 50.0->75.19428, order_gain 4.0->5.52885, mode_tau 30.0->29.52901, line_recency 0.6->0.55225, winners3 3->4, separation 3.0->0.5762, spread_plastic 0.5->0.4163 |
| c140 | 9.28 | 2 | 7.0 | 100 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, istdp_rate 50.0->122.96429, istdp_target 0.02->0.02085, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->4.17032, mode_tau 30.0->17.38749, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->4, channel_winners3 4->3, separation 3.0->3.5594, spread_plastic 0.5->0.4135 |
| c144 | 9.28 | 2 | 7.0 | 100 | learning_rate 0.04->0.03466, plastic_budget 4.0->3.66394, soft_bound 1.0->0.98407, presynaptic_bound 1.0->0.86613, istdp_rate 50.0->156.60028, encoding_suppression 1.0->0.99395, order_gain 4.0->5.52885, mode_tau 30.0->45.435, fatigue_gain3 0.0638->0.07424, line_recency 0.6->0.66799, winners3 3->5, channel_winners3 4->3, separation 3.0->0.5762, spread_plastic 0.5->0.1191 |
| c131 | 9.25 | 6 | 7.0 | 96 | plastic_budget 4.0->3.5517, soft_bound 1.0->0.75042, hetero_ltd 1.0->0.80485, istdp_rate 50.0->74.60277, istdp_target 0.02->0.01731, encoding_suppression 1.0->0.97447, consolidation_rate 0.0->0.06469, order_gain 4.0->4.7958, mode_tau 30.0->37.91201, fatigue_gain3 0.0638->0.05389, fire_threshold3 0.0256->0.02495, line_recency 0.6->0.55852, winners3 3->5, channel_winners3 4->3, separation 3.0->4.07163 |
| c136 | 9.24 | 4 | 7.0 | 91 | soft_bound 1.0->0.77136, presynaptic_bound 1.0->0.86613, consolidation_rate 0.0->0.00891, order_gain 4.0->6.66535, fire_threshold3 0.0256->0.02244, line_recency 0.6->0.54086, winners3 3->4, channel_winners3 4->3, separation 3.0->0.5762 |
| c148 | 9.22 | 2 | 7.0 | 100 | learning_rate 0.04->0.04229, plastic_budget 4.0->4.7963, istdp_rate 50.0->122.96429, istdp_target 0.02->0.02741, consolidated_budget 0.5->0.96786, order_gain 4.0->3.9856, order_tau 5.0->4.17032, mode_tau 30.0->17.38749, fire_threshold3 0.0256->0.02056, line_recency 0.6->0.6994, winners3 3->5, channel_winners3 4->3, separation 3.0->4.22114, spread_plastic 0.5->0.29681 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 2 | 24 |
| capacity | 2 | 24 |
| efficiency | 1 | 24 |
| wordpairs | 1 | 24 |
| continual | 6 | 24 |
| retention | 3 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 71% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): m>q: 1, z>q: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>river: 4, plant>river: 2, light>river: 1, cloud>candy: 1, storm>river: 1

## Picture so far (all generations together)

- Brain tests so far: 456 (156 versions, generation 19).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 43 | 456 | 0 | 38 |
| capacity | 74 | 456 | 5 | 38 |
| efficiency | 16 | 456 | 1 | 38 |
| wordpairs | 51 | 456 | 1 | 38 |
| continual | 85 | 456 | 4 | 38 |
| retention | 93 | 456 | 4 | 38 |
| order | 2 | 456 | 0 | 38 |

- Old memories getting worse (all tests): wiped out -0.015, pushed aside by new memories +0.012 on average; of 85 failed 'keep old memories' tests, 64 were mainly pushed aside (interference), 21 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 3107, stored but NOT recalled 526, not stored 15.
- Big-load probe (true word recall %, mean over brains): c1 32 words 68% (n=16); c1 64 words 39% (n=16); c10 32 words 78% (n=2); c10 64 words 44% (n=2); c11 32 words 44% (n=2); c11 64 words 19% (n=2); c112 32 words 80% (n=6); c112 64 words 55% (n=6); c114 32 words 72% (n=2); c114 64 words 45% (n=2); c12 32 words 75% (n=2); c12 64 words 41% (n=2); c131 32 words 80% (n=4); c131 64 words 49% (n=4); c134 32 words 75% (n=4); c134 64 words 45% (n=4); c136 32 words 75% (n=2); c136 64 words 44% (n=2); c142 32 words 81% (n=2); c142 64 words 50% (n=2); c146 32 words 75% (n=2); c146 64 words 48% (n=2); c26 32 words 62% (n=2); c26 64 words 31% (n=2); c27 32 words 59% (n=2); c27 64 words 22% (n=2); c30 32 words 31% (n=4); c30 64 words 9% (n=4); c31 32 words 75% (n=6); c31 64 words 45% (n=6); c36 32 words 59% (n=2); c36 64 words 30% (n=2); c38 32 words 84% (n=2); c38 64 words 47% (n=2); c4 32 words 56% (n=2); c4 64 words 30% (n=2); c44 32 words 84% (n=2); c44 64 words 36% (n=2); c45 32 words 66% (n=2); c45 64 words 31% (n=2); c46 32 words 58% (n=4); c46 64 words 40% (n=4); c48 32 words 56% (n=2); c48 64 words 34% (n=2); c52 32 words 66% (n=2); c52 64 words 23% (n=2); c57 32 words 75% (n=2); c57 64 words 31% (n=2); c76 32 words 78% (n=2); c76 64 words 34% (n=2); c79 32 words 75% (n=4); c79 64 words 46% (n=4); c83 32 words 75% (n=2); c83 64 words 41% (n=2); c89 32 words 91% (n=2); c89 64 words 52% (n=2); c92 32 words 72% (n=2); c92 64 words 38% (n=2); c94 32 words 91% (n=4); c94 64 words 55% (n=4); c95 32 words 69% (n=2); c95 64 words 38% (n=2); c96 32 words 81% (n=2); c96 64 words 52% (n=2); c99 32 words 82% (n=10); c99 64 words 51% (n=10); prime 32 words 71% (n=38); prime 64 words 31% (n=38)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.44 | +0.34 | +0.32 |
| inhibition_radius3 | -0.44 | -0.36 | -0.24 |
| order_gain | +0.32 | +0.19 | +0.18 |
| separation | -0.19 | -0.14 | -0.10 |
| consolidation_rate | -0.16 | -0.09 | -0.04 |
| winners3 | +0.15 | +0.13 | +0.07 |
| soft_bound | -0.14 | -0.05 | -0.03 |
| consolidated_budget | -0.13 | -0.00 | -0.05 |
| spread_plastic | +0.13 | +0.06 | +0.05 |
| learning_rate | -0.11 | -0.07 | -0.04 |
| line_recency | -0.11 | -0.00 | +0.00 |
| mode_tau | +0.10 | +0.02 | +0.07 |
| hetero_ltd | +0.08 | +0.08 | +0.04 |
| covariance | +0.07 | +0.05 | +0.11 |
| istdp_rate | -0.06 | +0.03 | +0.03 |
| assembly_inhibition | -0.06 | -0.12 | +0.05 |
| istdp_target | +0.05 | +0.06 | -0.04 |
| channel_winners3 | -0.05 | +0.02 | -0.04 |
| order_tau | +0.04 | -0.08 | -0.02 |
| plastic_budget | +0.03 | +0.02 | +0.04 |
| presynaptic_bound | -0.02 | -0.03 | -0.03 |
| fire_threshold3 | +0.02 | -0.02 | +0.01 |
| fatigue_gain3 | -0.00 | -0.03 | +0.04 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.