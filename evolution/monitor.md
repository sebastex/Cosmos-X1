# Evolution monitor (generation 3 done, 33.4 h left)

Score per brain = checks passed (of 7) + 2 x true word recall + 0.5 x memory filled in from a 40% cue - 0.25 per memory recalled as another. Means over every brain a candidate was tested on.

## Leaders
| candidate | score | brains | checks passed (mean) | word recall % (mean) | changes from Prime |
|---|---|---|---|---|---|
| c11 | 9.00 | 8 | 6.8 | 96 | plastic_budget 4.0->3.9007, assembly_inhibition 0.0->0.00063 |
| c33 | 8.95 | 2 | 6.5 | 100 | learning_rate 0.04->0.06267, plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.73385, istdp_rate 50.0->74.10519, istdp_target 0.02->0.02421, consolidation_rate 0.0->0.02862, consolidated_budget 0.5->0.86684, assembly_inhibition 0.0->0.00063, mode_tau 30.0->25.67333, fatigue_gain3 0.0638->0.07178, fire_threshold3 0.0256->0.03678, winners3 3->4, channel_winners3 4->3 |
| c29 | 8.71 | 2 | 6.5 | 94 | plastic_budget 4.0->4.16408, istdp_target 0.02->0.02722, fire_threshold3 0.0256->0.02666, winners3 3->4 |
| prime | 8.71 | 8 | 6.5 | 94 | (Prime) |
| c3 | 8.68 | 4 | 6.5 | 94 | learning_rate 0.04->0.0391, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917 |
| c27 | 8.67 | 2 | 6.5 | 88 | learning_rate 0.04->0.0391, covariance 1.0->0.89106, istdp_rate 50.0->80.95365, mode_tau 30.0->25.62044, line_recency 0.6->0.56917, channel_winners3 4->3 |
| c16 | 8.66 | 2 | 6.5 | 94 | plastic_budget 4.0->3.9007, hetero_ltd 1.0->0.86922, istdp_rate 50.0->74.10519, consolidation_rate 0.0->0.00569, assembly_inhibition 0.0->0.00063, fire_threshold3 0.0256->0.03678 |
| c32 | 8.65 | 2 | 6.5 | 88 | learning_rate 0.04->0.04686, plastic_budget 4.0->4.34164, fatigue_gain3 0.0638->0.10403 |

## Limitations seen this generation (all candidates)
| check | failed | of |
|---|---|---|
| recall | 1 | 24 |
| capacity | 8 | 24 |
| efficiency | 2 | 24 |
| wordpairs | 1 | 24 |
| continual | 9 | 24 |
| retention | 12 | 24 |
| order | 0 | 24 |

- A 40% cue brings back on average 70% of what the full letter brings (letters, 8 memories).
- Letters recalled as another memory (letter>taken for: times): m>e: 2, e>t: 2, m>a: 1, q>e: 1, t>w: 1, m>k: 1, w>k: 1, z>m: 1
- Word pairs recalled wrongly (cue>recalled: times): smile>candy: 9, cloud>candy: 2, cloud>water: 1, storm>dream: 1, light>house: 1, plant>candy: 1

## Picture so far (all generations together)

- Brain tests so far: 96 (36 versions, generation 4).

| check | failed | of | Prime failed | Prime of |
|---|---|---|---|---|
| recall | 7 | 96 | 0 | 8 |
| capacity | 18 | 96 | 0 | 8 |
| efficiency | 5 | 96 | 1 | 8 |
| wordpairs | 2 | 96 | 0 | 8 |
| continual | 26 | 96 | 2 | 8 |
| retention | 38 | 96 | 1 | 8 |
| order | 1 | 96 | 0 | 8 |

- Old memories getting worse (all tests): wiped out -0.020, pushed aside by new memories +0.033 on average; of 14 failed 'keep old memories' tests, 14 were mainly pushed aside (interference), 0 mainly wiped out (erasure).
- Word pairs (8 pairs per test, all tests): stored and recalled 353, stored but NOT recalled 31, not stored 0.
- Big-load probe (true word recall %, mean over brains): c11 32 words 75% (n=4); c11 64 words 35% (n=4); c21 32 words 66% (n=2); c21 64 words 39% (n=2); c26 32 words 69% (n=2); c26 64 words 34% (n=2); c3 32 words 81% (n=2); c3 64 words 36% (n=2); c9 32 words 69% (n=2); c9 64 words 22% (n=2); prime 32 words 72% (n=4); prime 64 words 37% (n=4)

### Which settings matter (correlation over all tests; |r| > 0.3 is worth a look)
| setting | with score | with keeping old memories (continual+retention) | with holding 8 memories |
|---|---|---|---|
| encoding_suppression | +0.42 | +0.22 | +0.32 |
| plastic_budget | -0.24 | -0.24 | -0.16 |
| covariance | +0.22 | +0.08 | +0.11 |
| inhibition_radius3 | -0.21 | -0.18 | -0.03 |
| hetero_ltd | +0.19 | +0.15 | +0.22 |
| fatigue_gain3 | +0.16 | +0.08 | +0.21 |
| soft_bound | +0.16 | +0.06 | +0.12 |
| winners3 | +0.16 | +0.16 | +0.03 |
| consolidation_rate | -0.15 | -0.15 | -0.15 |
| channel_winners3 | +0.12 | +0.17 | +0.09 |
| istdp_target | +0.12 | +0.05 | +0.06 |
| order_tau | +0.12 | -0.01 | +0.15 |
| istdp_rate | +0.09 | +0.15 | +0.01 |
| assembly_inhibition | -0.08 | -0.16 | +0.02 |
| order_gain | +0.07 | -0.05 | +0.14 |
| presynaptic_bound | -0.06 | +0.04 | -0.11 |
| fire_threshold3 | +0.04 | -0.02 | -0.11 |
| mode_tau | +0.03 | +0.07 | -0.02 |
| line_recency | -0.03 | +0.06 | -0.16 |
| learning_rate | +0.03 | -0.02 | +0.01 |
| consolidated_budget | -0.02 | +0.08 | +0.08 |

Reading: a failure that no setting moves (all correlations near 0) points to a missing part, not a missing tuning.