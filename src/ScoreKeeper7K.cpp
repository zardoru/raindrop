#include "pch.h"

#include "ScoreKeeper7K.h"

ScoreKeeper7K::~ScoreKeeper7K() {}

double ScoreKeeper7K::accuracy_percent(double var)
{
    return double(ACC_MAX_SQ - var) / (ACC_MAX_SQ - ACC_MIN_SQ) * 100;
}

void ScoreKeeper7K::setAccMin(double ms)
{
    ACC_MIN = ms;
    ACC_MIN_SQ = ms * ms;
}

void ScoreKeeper7K::setAccMax(double ms)
{
    ACC_MAX = ms;
    ACC_MAX_SQ = ms * ms;
}

void ScoreKeeper7K::setMaxNotes(int notes)
{
    max_notes = std::max(notes, 1);

    if (notes < 10) bms_max_combo_pts = notes * (notes + 1) / 2;
    else bms_max_combo_pts = 55 + (notes - 10) * 10;

    if (notes < 25) exp_max_combo_pts = notes * (notes + 1) * 2;
    else exp_max_combo_pts = 1300 + (notes - 25) * 100;
}

int ScoreKeeper7K::getMaxNotes() { return max_notes; }

int ScoreKeeper7K::getTotalNotes() { return total_notes; }

// ms is misleading- since it may very well be beats, but it's fine.
ScoreKeeperJudgment ScoreKeeper7K::hitNote(double ms)
{
    // hit notes

    avg_hit *= total_notes;
    ++total_notes;
    avg_hit += ms;
    avg_hit /= total_notes;

    // std::cerr << use_bbased << " " << ms << " ";

    if (use_bbased)
    {
        if (abs(ms * 150) < 128)
        {
            ++histogram[static_cast<int>(round(ms * 150)) + 127];
        }
    }
    else
    {
        if (abs(ms) < 128)
        {
            ++histogram[static_cast<int>(round(ms)) + 127];
        }
    }

    ms = abs(ms);

    // combo

    double jt;
    if (use_bbased)
        jt = judgment_time[SKJ_W2];
    else
        jt = judgment_time[SKJ_W3];

    if (ms <= jt)
    {
        ++notes_hit;
        ++combo;
        if (combo > max_combo)
            max_combo = combo;
    }
    else
    {
        combo = 0;
    }

    // accuracy score

    if (use_bbased)
        total_sqdev += ms * ms * 22500;
    else
        total_sqdev += ms * ms;

    accuracy = accuracy_percent(total_sqdev / total_notes);

    // judgments

    ScoreKeeperJudgment judgment = SKJ_NONE;

    for (int i = (use_w0 ? 0 : 1); i < (use_bbased ? 4 : 6); i++)
    {
        if (ms <= judgment_time[i])
        {
            judgment = ScoreKeeperJudgment(i);

            if (!use_bbased)
                judgment_amt[judgment]++;
            else
            {
                // we using o2 based mechanics..
                auto j = getO2Judge(judgment);

				// transform this judgment if neccesary (pills/etc)
				judgment = ScoreKeeperJudgment(j); 
                judgment_amt[j]++;
            }

			// breaking early is important.
            break;
        }
    }

    // SC, ACC^2 score

    sc_score += Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0) * 2;
    sc_sc_score += sc_score * Clamp(accuracy_percent(ms * ms) / 100, 0.0, 1.0);

    score = double(SCORE_MAX * sc_sc_score) / (max_notes * (max_notes + 1));

    // lifebars

    if (ms <= judgment_time[SKJ_W3])
    {
        lifebar_easy = std::min(1.0, lifebar_easy + lifebar_easy_increment);
        lifebar_groove = std::min(1.0, lifebar_groove + lifebar_groove_increment);

        if (lifebar_survival > 0)
            lifebar_survival = std::min(1.0, lifebar_survival + lifebar_survival_increment);
        if (lifebar_exhard > 0)
            lifebar_exhard = std::min(1.0, lifebar_exhard + lifebar_exhard_increment);
    }
    else
    {
        // miss tier 1
        lifebar_easy = std::max(0.0, lifebar_easy - lifebar_easy_decrement);
        lifebar_groove = std::max(0.0, lifebar_groove - lifebar_groove_decrement);
        lifebar_survival = std::max(0.0, lifebar_survival - lifebar_groove_decrement);
        lifebar_exhard = std::max(0.0, lifebar_exhard - lifebar_exhard_decrement);

        lifebar_death = 0;
    }

    if (ms <= judgment_time[SKJ_W1])
    { // only COOLs restore o2jam lifebar
        lifebar_o2jam = std::min(1.0, lifebar_o2jam + lifebar_o2jam_increment);
    }
    else if (ms > judgment_time[SKJ_W2]) // BADs get some HP from you,
        lifebar_o2jam = std::max(0.0, lifebar_o2jam - lifebar_o2jam_decrement_bad);

    // std::cerr << ms << " " << judgment << " " << life_increment[judgment] << std::endl;

    lifebar_stepmania = std::min(1.0, lifebar_stepmania + life_increment[judgment]);

    if (judgment == SKJ_NONE)
        std::cerr << "Error, invalid judgment: " << ms << "\n";

    // std::cerr << std::endl;

// Other methods

    update_ranks(judgment); // rank calculation
    update_bms(judgment); // Beatmania scoring
    update_lr2(judgment); // Lunatic Rave 2 scoring
    update_exp2(judgment);
    update_osu(judgment);
    update_o2(judgment);

    return judgment;
}

int ScoreKeeper7K::getJudgmentCount(int judgment)
{
    if (judgment >= 9 || judgment < 0) return 0;

    return judgment_amt[judgment];
}

bool ScoreKeeper7K::usesW0() { return use_w0; }

void ScoreKeeper7K::missNote(bool auto_hold_miss, bool early_miss)
{
    judgment_amt[SKJ_MISS]++;

    if (!early_miss)
        ++total_notes;

    accuracy = accuracy_percent(total_sqdev / total_notes);

    if (!early_miss)
    {
		if (!auto_hold_miss) {
			total_sqdev += getMissCutoff() * getMissCutoff();
			combo = 0;
		}

        // miss tier 2
        lifebar_easy = std::max(0.0, lifebar_easy - lifebar_easy_decrement * 3);
        lifebar_groove = std::max(0.0, lifebar_groove - lifebar_groove_decrement * 3);
        lifebar_survival = std::max(0.0, lifebar_survival - lifebar_survival_decrement * 3);
        lifebar_exhard = std::max(0.0, lifebar_exhard - lifebar_exhard_decrement * 3);

        lifebar_death = 0;

        lifebar_stepmania = std::max(0.0, lifebar_stepmania - lifebar_stepmania_miss_decrement);

        lifebar_o2jam = std::max(0.0, lifebar_o2jam - lifebar_o2jam_decrement);
    }
    else if (early_miss)
    {
        // miss tier 1
        lifebar_easy = std::max(0.0, lifebar_easy - lifebar_easy_decrement);
        lifebar_groove = std::max(0.0, lifebar_groove - lifebar_groove_decrement);
        lifebar_survival = std::max(0.0, lifebar_survival - lifebar_survival_decrement);
        lifebar_exhard = std::max(0.0, lifebar_exhard - lifebar_exhard_decrement);

        lifebar_stepmania = std::max(0.0, lifebar_stepmania - lifebar_stepmania_earlymiss_decrement);
	}

    // other methods
    update_bms(SKJ_MISS);
    update_exp2(SKJ_MISS);
    update_osu(SKJ_MISS);
    update_o2(SKJ_MISS);
}

double ScoreKeeper7K::getJudgmentCutoff() {
	auto rt = 0.0;
	for (int i = 0; i <= SKJ_MISS; i++)
		rt = std::max(judgment_time[i], rt);

	rt = std::max(std::max(miss_threshold, rt), earlymiss_threshold);
	return rt;
}

double ScoreKeeper7K::getEarlyMissCutoff()
{
    return earlymiss_threshold;
}

double ScoreKeeper7K::getMissCutoff()
{
    return miss_threshold;
}

double ScoreKeeper7K::getAccMax()
{
    return ACC_MAX;
}

double ScoreKeeper7K::getJudgmentWindow(int judgment)
{
    if (judgment >= 9 || judgment < 0) return 0;
    return judgment_time[judgment];
}

std::string ScoreKeeper7K::getHistogram()
{
    std::stringstream ss;

    const int HISTOGRAM_DISPLAY_WIDTH = 15;

    for (int i = 0; i < 255; ++i)
    {
        int it = (i % HISTOGRAM_DISPLAY_WIDTH) * (255 / HISTOGRAM_DISPLAY_WIDTH) + (i / HISTOGRAM_DISPLAY_WIDTH); // transpose
        ss << std::setw(4) << it - 127 << ": " << std::setw(4) << histogram[it] << " ";
        if (i % HISTOGRAM_DISPLAY_WIDTH == HISTOGRAM_DISPLAY_WIDTH - 1)
            ss << "\n";
    }

    return ss.str();
}

int ScoreKeeper7K::getHistogramPoint(int point)
{
    int msCount = sizeof(histogram) / sizeof(double) / 2;
    if (abs(point) > msCount) return 0;
    return histogram[point + msCount];
}

int ScoreKeeper7K::getHistogramPointCount()
{
    return sizeof(histogram) / sizeof(double);
}

int ScoreKeeper7K::getHistogramHighestPoint()
{
    return std::accumulate(&histogram[0], histogram + getHistogramPointCount(), 1.0, [](double a, double b) -> double
    {
        return std::max(a, b);
    });
}

double ScoreKeeper7K::getAvgHit()
{
    return avg_hit;
}

/* actual score functions. */

int ScoreKeeper7K::getScore(int score_type)
{
    switch (score_type)
    {
    case ST_SCORE:
        return int(score);
    case ST_EX:
        return ex_score;
    case ST_IIDX:
        return bms_score;
    case ST_LR2:
        return lr2_score;
    case ST_EXP:
        return exp_score;
    case ST_EXP3:
        return exp3_score;
    case ST_OSUMANIA:
        return osu_score;
    case ST_COMBO:
        return combo;
    case ST_MAX_COMBO:
        return max_combo;
    case ST_NOTES_HIT:
        return notes_hit;
    case ST_O2JAM:
        return o2_score;
    default:
        return 0;
    }
}

float ScoreKeeper7K::getPercentScore(int percent_score_type) {
	switch (percent_score_type) {
	case PST_RANK:
		if (total_notes)
			return double(rank_pts) / double(total_notes) * 100.0;
		return 100;
	case PST_EX:
		if (total_notes)
			return double(ex_score) / double(total_notes * 2) * 100.0;
		return 100;
	case PST_ACC:
		return accuracy;
	case PST_NH:
		if (total_notes)
			return double(notes_hit) / double(total_notes) * 100.0;
		return 100;
	case PST_OSU:
		if (total_notes)
			return double(osu_accuracy) / double(total_notes) / 3.0;
		return 100;
	default:
		return 0;
	}
}

int ScoreKeeper7K::getLifebarUnits(int lifebar_unit_type)
{
    return 0;
}

float ScoreKeeper7K::getLifebarAmount(int lifebar_amount_type)
{
    switch (lifebar_amount_type)
    {
    case LT_EASY:
        return lifebar_easy;
    case LT_GROOVE:
        return lifebar_groove;
    case LT_SURVIVAL:
        return lifebar_survival;
    case LT_EXHARD:
        return lifebar_exhard;
    case LT_DEATH:
        return lifebar_death;
    case LT_STEPMANIA:
        return lifebar_stepmania;
    case LT_O2JAM:
        return lifebar_o2jam;
    default:
        return 0;
    }
}

bool ScoreKeeper7K::isStageFailed(int lifebar_amount_type)
{
    switch (lifebar_amount_type)
    {
    case LT_GROOVE:
        return total_notes == max_notes && lifebar_groove < 0.80;
    case LT_EASY:
        return total_notes == max_notes && lifebar_easy < 0.80;
    case LT_SURVIVAL:
        return lifebar_survival <= 0.0;
    case LT_EXHARD:
        return lifebar_exhard <= 0.0;
    case LT_DEATH:
        return lifebar_death <= 0.0;
    case LT_STEPMANIA:
        return lifebar_stepmania <= 0.0;
    case LT_O2JAM:
        return lifebar_o2jam <= 0.0;
    default:
        return false;
    }
}

void ScoreKeeper7K::failStage()
{
    total_notes = max_notes;

    update_ranks(SKJ_MISS); // rank calculation
    update_bms(SKJ_MISS); // Beatmania scoring
    update_lr2(SKJ_MISS); // Lunatic Rave 2 scoring
    update_exp2(SKJ_MISS);
    update_osu(SKJ_MISS);
}

int ScoreKeeper7K::getPacemakerDiff(PacemakerType pacemaker)
{
    switch (pacemaker)
    {
    case PMT_F:
        return ex_score - (total_notes * 2 / 9 + (total_notes * 2 % 9 != 0));
    case PMT_E:
        return ex_score - (total_notes * 4 / 9 + (total_notes * 4 % 9 != 0));
    case PMT_D:
        return ex_score - (total_notes * 6 / 9 + (total_notes * 6 % 9 != 0));
    case PMT_C:
        return ex_score - (total_notes * 8 / 9 + (total_notes * 8 % 9 != 0));
    case PMT_B:
        return ex_score - (total_notes * 10 / 9 + (total_notes * 10 % 9 != 0));
    case PMT_A:
        return ex_score - (total_notes * 12 / 9 + (total_notes * 12 % 9 != 0));
    case PMT_AA:
        return ex_score - (total_notes * 14 / 9 + (total_notes * 14 % 9 != 0));
    case PMT_AAA:
        return ex_score - (total_notes * 16 / 9 + (total_notes * 16 % 9 != 0));

    case PMT_50EX:
        return ex_score - (total_notes);
    case PMT_75EX:
        return ex_score - (total_notes * 75 / 50);
    case PMT_85EX:
        return ex_score - (total_notes * 85 / 50);

    case PMT_RANK_ZERO:
        return rank_pts - (total_notes * 100 / 100);
    case PMT_RANK_P1:
        return rank_pts - (total_notes * 120 / 100 + (total_notes * 120 % 100 != 0));
    case PMT_RANK_P2:
        return rank_pts - (total_notes * 140 / 100 + (total_notes * 140 % 100 != 0));
    case PMT_RANK_P3:
        return rank_pts - (total_notes * 160 / 100 + (total_notes * 160 % 100 != 0));
    case PMT_RANK_P4:
        return rank_pts - (total_notes * 180 / 100 + (total_notes * 180 % 100 != 0));
    case PMT_RANK_P5:
        return rank_pts - (total_notes * 200 / 100);
    case PMT_RANK_P6:
        return rank_pts - (total_notes * 220 / 100 + (total_notes * 220 % 100 != 0));
    case PMT_RANK_P7:
        return rank_pts - (total_notes * 240 / 100 + (total_notes * 240 % 100 != 0));
    case PMT_RANK_P8:
        return rank_pts - (total_notes * 260 / 100 + (total_notes * 260 % 100 != 0));
    case PMT_RANK_P9:
        return rank_pts - (total_notes * 280 / 100 + (total_notes * 280 % 100 != 0));
    }

    return 0;
}

std::pair<std::string, int> ScoreKeeper7K::getAutoPacemaker()
{
    PacemakerType pmt;

    if (ex_score < total_notes * 2 / 9)  pmt = PMT_F;
    else if (ex_score < total_notes * 5 / 9)  pmt = PMT_E;
    else if (ex_score < total_notes * 7 / 9)  pmt = PMT_D;
    else if (ex_score < total_notes * 9 / 9)  pmt = PMT_C;
    else if (ex_score < total_notes * 11 / 9)  pmt = PMT_B;
    else if (ex_score < total_notes * 13 / 9)  pmt = PMT_A;
    else if (ex_score < total_notes * 15 / 9)  pmt = PMT_AA;
    else  pmt = PMT_AAA;

    int pacemaker = getPacemakerDiff(pmt);
    std::stringstream ss;
    ss
        << std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

    return std::make_pair(ss.str(), pacemaker);
}

std::pair<std::string, int> ScoreKeeper7K::getAutoRankPacemaker()
{
    PacemakerType pmt;
    if (rank_pts < total_notes * 110 / 100)  pmt = PMT_RANK_ZERO;
    else if (rank_pts < total_notes * 130 / 100)  pmt = PMT_RANK_P1;
    else if (rank_pts < total_notes * 150 / 100)  pmt = PMT_RANK_P2;
    else if (rank_pts < total_notes * 170 / 100)  pmt = PMT_RANK_P3;
    else if (rank_pts < total_notes * 190 / 100)  pmt = PMT_RANK_P4;
    else if (rank_pts < total_notes * 210 / 100)  pmt = PMT_RANK_P5;
    else if (rank_pts < total_notes * 230 / 100)  pmt = PMT_RANK_P6;
    else if (rank_pts < total_notes * 250 / 100)  pmt = PMT_RANK_P7;
    else if (rank_pts < total_notes * 270 / 100)  pmt = PMT_RANK_P8;
    else  pmt = PMT_RANK_P9;

    int pacemaker = getPacemakerDiff(pmt);

    std::stringstream ss;
    ss
        << std::setfill(' ') << std::setw(4) << pacemaker_texts[pmt] << ": ";

    return std::make_pair(ss.str(), pacemaker);
}