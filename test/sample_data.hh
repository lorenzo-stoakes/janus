#pragma once

#include <array>
#include <string>

namespace janus_test
{
static const char* sample_meta_json =
	R"({"marketId":"1.170020941","marketName":"1m2f Hcap","marketStartTime":"2020-03-11T13:20:00Z","description":{"persistenceEnabled":true,"bspMarket":true,"marketTime":"2020-03-11T13:20:00Z","suspendTime":"2020-03-11T13:20:00Z","bettingType":"ODDS","turnInPlayEnabled":true,"marketType":"WIN","regulator":"GIBRALTAR REGULATOR","marketBaseRate":5,"discountAllowed":true,"wallet":"UK wallet","rules":"<br><a href=\"https://www.timeform.com/horse-racing/\" target=\"_blank\"><img src=\" http://content-cache.betfair.com/images/en_GB/mr_fr.gif\" title=”Form/ Results” border=\"0\"></a>\n<br><br><b>MARKET INFORMATION</b><br><br>This market information applies to Exchange Singles bets only. Please see our <a href=http://content.betfair.com/aboutus/content.asp?sWhichKey=Rules%20and%20Regulations#undefined.do style=color:0163ad; text-decoration: underline; target=_blank>Rules & Regs</a> for further information, multiples and all other sections.<br><br>Who will win this race? Betfair Non-Runner Rule applies. This market will turn IN PLAY at the off with unmatched bets (with the exception of bets for which the \"keep\" option has been selected) cancelled once the Betfair SP reconciliation process has been completed. Betting will be suspended at the end of the race. Should there be a very close finish, a photo finish declared, a Stewards Enquiry or an Objection called the market may be re-opened with unmatched bets (again with the exception of bets for which the \"keep\" option has been selected) cancelled. The market will then be suspended when this result is announced. This market will initially be settled on a First Past the Post basis. However we will re-settle all bets should the official result at the time of the \"weigh-in\" announcement differ from any initial settlement. BETS ARE PLACED ON A NAMED HORSE. Dead Heat rules apply.<br><br>Customers should be aware that:<b><ol><li>transmissions described as \"live\" by some broadcasters may actually be delayed;</li><li>the extent of any such delay may vary, depending on the set-up through which they are receiving pictures or data;</b> and </li><li>information (such as jockey silks, saddlecloth numbers etc) is provided \"as is\" and is for guidance only. Betfair does not guarantee the accuracy of this information and use of it to place bets is entirely at your own risk.</li></ol><br>","rulesHasDate":true,"clarifications":"NR: (UKT) <br> 11. Rocksette (0.0%,18:58"},"totalMatched":11165.34,"runners":[{"selectionId":13233309,"runnerName":"Zayriyan","sortPriority":1,"metadata":{"ADJUSTED_RATING":"0","AGE":"5","BRED":"IRE","CLOTH_NUMBER":"9","CLOTH_NUMBER_ALPHA":"9","COLOURS_DESCRIPTION":"Black and yellow check, red sleeves and cap","COLOURS_FILENAME":"c20200311lin/00081124A.jpg","COLOUR_TYPE":"ch","DAMSIRE_BRED":"IRE","DAMSIRE_NAME":"Marju","DAMSIRE_YEAR_BORN":"1988","DAM_BRED":"IRE","DAM_NAME":"Zariyna","DAM_YEAR_BORN":"2009","DAYS_SINCE_LAST_RUN":"13","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"4","FORM":"7050-72","JOCKEY_CLAIM":"","JOCKEY_NAME":"Tom Marquand","OFFICIAL_RATING":"55","OWNER_NAME":"Friends Of Castle Piece","SEX_TYPE":"g","SIRE_BRED":"USA","SIRE_NAME":"Shamardal","SIRE_YEAR_BORN":"2002","STALL_DRAW":"13","TRAINER_NAME":"Ali Stronge","WEARING":"tongue strap","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"128","runnerId":"13233309"}},{"selectionId":11622845,"runnerName":"Abel Tasman","sortPriority":2,"metadata":{"ADJUSTED_RATING":"0","AGE":"6","BRED":"GB","CLOTH_NUMBER":"3","CLOTH_NUMBER_ALPHA":"3","COLOURS_DESCRIPTION":"Royal blue, yellow stars, yellow and royal blue check sleeves, royal blue cap, yellow star","COLOURS_FILENAME":"c20200311lin/00078447.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"USA","DAMSIRE_NAME":"Sadler's Wells","DAMSIRE_YEAR_BORN":"1981","DAM_BRED":"IRE","DAM_NAME":"Helena Molony","DAM_YEAR_BORN":"2002","DAYS_SINCE_LAST_RUN":"51","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"20","FORM":"0/079-80","JOCKEY_CLAIM":"","JOCKEY_NAME":"Richard Kingscote","OFFICIAL_RATING":"60","OWNER_NAME":"Mr Ian Furlong","SEX_TYPE":"g","SIRE_BRED":"UK","SIRE_NAME":"Mount Nelson","SIRE_YEAR_BORN":"2004","STALL_DRAW":"12","TRAINER_NAME":"Ian Williams","WEARING":"","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"133","runnerId":"11622845"}},{"selectionId":17247906,"runnerName":"Huddle","sortPriority":3,"metadata":{"ADJUSTED_RATING":"0","AGE":"4","BRED":"GB","CLOTH_NUMBER":"6","CLOTH_NUMBER_ALPHA":"6","COLOURS_DESCRIPTION":"Light blue, dark blue inverted triangle","COLOURS_FILENAME":"c20200311lin/00079230.jpg","COLOUR_TYPE":"gr","DAMSIRE_BRED":"USA","DAMSIRE_NAME":"Shamardal","DAMSIRE_YEAR_BORN":"2002","DAM_BRED":"UK","DAM_NAME":"Purest","DAM_YEAR_BORN":"2009","DAYS_SINCE_LAST_RUN":"25","FORECASTPRICE_DENOMINATOR":"2","FORECASTPRICE_NUMERATOR":"11","FORM":"7106-44","JOCKEY_CLAIM":"","JOCKEY_NAME":"Callum Shepherd","OFFICIAL_RATING":"58","OWNER_NAME":"Mr & Mrs N. Welby","SEX_TYPE":"f","SIRE_BRED":"USA","SIRE_NAME":"Aussie Rules","SIRE_YEAR_BORN":"2003","STALL_DRAW":"5","TRAINER_NAME":"William Knight","WEARING":"visor","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"131","runnerId":"17247906"}},{"selectionId":12635885,"runnerName":"Muraaqeb","sortPriority":4,"metadata":{"ADJUSTED_RATING":"0","AGE":"6","BRED":"GB","CLOTH_NUMBER":"7","CLOTH_NUMBER_ALPHA":"7","COLOURS_DESCRIPTION":"Yellow, black star, quartered cap","COLOURS_FILENAME":"c20200311lin/00001946.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"USA","DAMSIRE_NAME":"Danehill","DAMSIRE_YEAR_BORN":"1986","DAM_BRED":"UK","DAM_NAME":"Tesary","DAM_YEAR_BORN":"2002","DAYS_SINCE_LAST_RUN":"14","FORECASTPRICE_DENOMINATOR":"2","FORECASTPRICE_NUMERATOR":"11","FORM":"431585","JOCKEY_CLAIM":"","JOCKEY_NAME":"Luke Morris","OFFICIAL_RATING":"57","OWNER_NAME":"Mr E. A. Hayward","SEX_TYPE":"g","SIRE_BRED":"IRE","SIRE_NAME":"Nathaniel","SIRE_YEAR_BORN":"2008","STALL_DRAW":"11","TRAINER_NAME":"Milton Bradley","WEARING":"cheekpieces","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"130","runnerId":"12635885"}},{"selectionId":10322409,"runnerName":"Doras Field","sortPriority":5,"metadata":{"ADJUSTED_RATING":"0","AGE":"7","BRED":"IRE","CLOTH_NUMBER":"2","CLOTH_NUMBER_ALPHA":"2","COLOURS_DESCRIPTION":"Dark green, yellow stars, hooped sleeves, yellow cap","COLOURS_FILENAME":"c20200311lin/00040320.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"IRE","DAMSIRE_NAME":"Cape Cross","DAMSIRE_YEAR_BORN":"1994","DAM_BRED":"IRE","DAM_NAME":"Rydal Mount","DAM_YEAR_BORN":"2003","DAYS_SINCE_LAST_RUN":"539","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"10","FORM":"78410/","JOCKEY_CLAIM":"","JOCKEY_NAME":"Kieran Shoemark","OFFICIAL_RATING":"62","OWNER_NAME":"Mr Reg Gifford","SEX_TYPE":"m","SIRE_BRED":"IRE","SIRE_NAME":"Rip Van Winkle","SIRE_YEAR_BORN":"2006","STALL_DRAW":"9","TRAINER_NAME":"Stuart Kittow","WEARING":"tongue strap","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"135","runnerId":"10322409"}},{"selectionId":15062473,"runnerName":"Arlecchinos Arc","sortPriority":6,"metadata":{"ADJUSTED_RATING":"0","AGE":"5","BRED":"IRE","CLOTH_NUMBER":"8","CLOTH_NUMBER_ALPHA":"8","COLOURS_DESCRIPTION":"Dark green, maroon sleeves, white spots, maroon cap, white spots","COLOURS_FILENAME":"c20200311lin/00829759.jpg","COLOUR_TYPE":"ch","DAMSIRE_BRED":"USA","DAMSIRE_NAME":"Thunder Gulch","DAMSIRE_YEAR_BORN":"1992","DAM_BRED":"IRE","DAM_NAME":"Sir Cecil's Girl","DAM_YEAR_BORN":"2008","DAYS_SINCE_LAST_RUN":"13","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"12","FORM":"621-347","JOCKEY_CLAIM":"","JOCKEY_NAME":"David Probert","OFFICIAL_RATING":"56","OWNER_NAME":"Mr K. Senior","SEX_TYPE":"g","SIRE_BRED":"IRE","SIRE_NAME":"Arcano","SIRE_YEAR_BORN":"2007","STALL_DRAW":"2","TRAINER_NAME":"Mark Usher","WEARING":"visor","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"129","runnerId":"15062473"}},{"selectionId":18889965,"runnerName":"Flying Focus","sortPriority":7,"metadata":{"ADJUSTED_RATING":"0","AGE":"5","BRED":"IRE","CLOTH_NUMBER":"13","CLOTH_NUMBER_ALPHA":"13","COLOURS_DESCRIPTION":"Orange, dark blue epaulets, dark blue and orange halved sleeves, orange cap, dark blue diamond","COLOURS_FILENAME":"c20200311lin/00069760.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"USA","DAMSIRE_NAME":"Catrail","DAMSIRE_YEAR_BORN":"1990","DAM_BRED":"IRE","DAM_NAME":"Cat Belling","DAM_YEAR_BORN":"2000","DAYS_SINCE_LAST_RUN":"13","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"7","FORM":"5/000-26","JOCKEY_CLAIM":"","JOCKEY_NAME":"Rob Hornby","OFFICIAL_RATING":"48","OWNER_NAME":"M. F. Harris","SEX_TYPE":"g","SIRE_BRED":"USA","SIRE_NAME":"The Carbon Unit","SIRE_YEAR_BORN":"2002","STALL_DRAW":"8","TRAINER_NAME":"Milton Harris","WEARING":"cheekpieces","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"121","runnerId":"18889965"}},{"selectionId":10220888,"runnerName":"Fivehundredmiles","sortPriority":8,"metadata":{"ADJUSTED_RATING":"0","AGE":"7","BRED":"IRE","CLOTH_NUMBER":"10","CLOTH_NUMBER_ALPHA":"10","COLOURS_DESCRIPTION":"Yellow, royal blue triple diamond, diabolo on sleeves, royal blue cap, yellow diamond","COLOURS_FILENAME":"c20200311lin/00861511.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"USA","DAMSIRE_NAME":"Fantastic Light","DAMSIRE_YEAR_BORN":"1996","DAM_BRED":"IRE","DAM_NAME":"There's A Light","DAM_YEAR_BORN":"2004","DAYS_SINCE_LAST_RUN":"19","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"18","FORM":"600-407","JOCKEY_CLAIM":"","JOCKEY_NAME":"Ben Curtis","OFFICIAL_RATING":"53","OWNER_NAME":"Mrs Olena Tarnavska","SEX_TYPE":"g","SIRE_BRED":"USA","SIRE_NAME":"The Carbon Unit","SIRE_YEAR_BORN":"2002","STALL_DRAW":"1","TRAINER_NAME":"David Evans","WEARING":"visor","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"126","runnerId":"10220888"}},{"selectionId":22109331,"runnerName":"Mrs Meader","sortPriority":9,"metadata":{"ADJUSTED_RATING":"0","AGE":"4","BRED":"GB","CLOTH_NUMBER":"1","CLOTH_NUMBER_ALPHA":"1","COLOURS_DESCRIPTION":"Black, yellow stars, red sleeves, black cap, yellow star","COLOURS_FILENAME":"c20200311lin/00855580A.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"UK","DAMSIRE_NAME":"Dansili","DAMSIRE_YEAR_BORN":"1996","DAM_BRED":"UK","DAM_NAME":"Bavarica","DAM_YEAR_BORN":"2002","DAYS_SINCE_LAST_RUN":"224","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"18","FORM":"445634-","JOCKEY_CLAIM":"","JOCKEY_NAME":"Kieran O'Neill","OFFICIAL_RATING":"62","OWNER_NAME":"NJ Bloodstock","SEX_TYPE":"f","SIRE_BRED":"UK","SIRE_NAME":"Cityscape","SIRE_YEAR_BORN":"2006","STALL_DRAW":"7","TRAINER_NAME":"Seamus Mullins","WEARING":"hood","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"135","runnerId":"22109331"}},{"selectionId":24270884,"runnerName":"Thats A Shame","sortPriority":10,"metadata":{"ADJUSTED_RATING":"0","AGE":"5","BRED":"IRE","CLOTH_NUMBER":"5","CLOTH_NUMBER_ALPHA":"5","COLOURS_DESCRIPTION":"Mauve and black (quartered), mauve sleeves, white cap","COLOURS_FILENAME":"c20200311lin/00079926.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"IRE","DAMSIRE_NAME":"Imperial Ballet","DAMSIRE_YEAR_BORN":"1989","DAM_BRED":"IRE","DAM_NAME":"World of Ballet","DAM_YEAR_BORN":"2003","DAYS_SINCE_LAST_RUN":"21","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"20","FORM":"646","JOCKEY_CLAIM":"","JOCKEY_NAME":"Hector Crouch","OFFICIAL_RATING":"59","OWNER_NAME":"Galloping On The South Downs Partnership","SEX_TYPE":"g","SIRE_BRED":"GER","SIRE_NAME":"Arcadio","SIRE_YEAR_BORN":"2002","STALL_DRAW":"6","TRAINER_NAME":"Gary Moore","WEARING":"","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"132","runnerId":"24270884"}},{"selectionId":13229196,"runnerName":"Temujin","sortPriority":11,"metadata":{"ADJUSTED_RATING":"0","AGE":"4","BRED":"IRE","CLOTH_NUMBER":"4","CLOTH_NUMBER_ALPHA":"4","COLOURS_DESCRIPTION":"Emerald green, red disc, emerald green sleeves, red stars, emerald green cap, red spots","COLOURS_FILENAME":"c20200311lin/00857449.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"IRE","DAMSIRE_NAME":"Alhaarth","DAMSIRE_YEAR_BORN":"1993","DAM_BRED":"IRE","DAM_NAME":"Alhena","DAM_YEAR_BORN":"2007","DAYS_SINCE_LAST_RUN":"6","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"33","FORM":"228-088","JOCKEY_CLAIM":"","JOCKEY_NAME":"Alistair Rawlinson","OFFICIAL_RATING":"59","OWNER_NAME":"Rockingham Reins Limited","SEX_TYPE":"g","SIRE_BRED":"IRE","SIRE_NAME":"Moohaajim","SIRE_YEAR_BORN":"2010","STALL_DRAW":"3","TRAINER_NAME":"Michael Appleby","WEARING":"visor and tongue strap","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"132","runnerId":"13229196"}},{"selectionId":12481874,"runnerName":"Rocksette","sortPriority":12,"metadata":{"ADJUSTED_RATING":"0","AGE":"6","BRED":"GB","CLOTH_NUMBER":"11","CLOTH_NUMBER_ALPHA":"11","COLOURS_DESCRIPTION":"Purple, yellow triple diamond, white sleeves, yellow diamonds, purple cap, yellow diamond","COLOURS_FILENAME":"c20200311lin/00841933.jpg","COLOUR_TYPE":"b","DAMSIRE_BRED":"USA","DAMSIRE_NAME":"Be My Native","DAMSIRE_YEAR_BORN":"1979","DAM_BRED":"IRE","DAM_NAME":"Native Nickel","DAM_YEAR_BORN":"1997","DAYS_SINCE_LAST_RUN":"5","FORECASTPRICE_DENOMINATOR":"1","FORECASTPRICE_NUMERATOR":"66","FORM":"08880-8","JOCKEY_CLAIM":"","JOCKEY_NAME":"Robert Havlin","OFFICIAL_RATING":"50","OWNER_NAME":"Hide & Seekers","SEX_TYPE":"m","SIRE_BRED":"UK","SIRE_NAME":"Mount Nelson","SIRE_YEAR_BORN":"2004","STALL_DRAW":"4","TRAINER_NAME":"Adam West","WEARING":"blinkers","WEIGHT_UNITS":"pounds","WEIGHT_VALUE":"123","runnerId":"12481874"}}],"eventType":{"id":"7","name":"Horse Racing"},"competition":{"id":"","name":""},"event":{"id":"29746086","name":"Ling  11th Mar","countryCode":"GB","timezone":"Europe/London","venue":"Lingfield","openDate":"2020-03-11T13:20:00Z"}})";

static const std::array<uint64_t, 12> sample_runner_ids = {
	13233309, 11622845, 17247906, 12635885, 10322409, 15062473,
	18889965, 10220888, 22109331, 24270884, 13229196, 12481874,
};

static const std::array<std::string, 12> sample_runner_names = {
	"Zayriyan",    "Abel Tasman",     "Huddle",       "Muraaqeb",
	"Doras Field", "Arlecchinos Arc", "Flying Focus", "Fivehundredmiles",
	"Mrs Meader",  "Thats A Shame",   "Temujin",      "Rocksette",
};

static const std::array<std::string, 12> sample_jockey_names = {
	"Tom Marquand",    "Richard Kingscote", "Callum Shepherd",    "Luke Morris",
	"Kieran Shoemark", "David Probert",     "Rob Hornby",         "Ben Curtis",
	"Kieran O'Neill",  "Hector Crouch",     "Alistair Rawlinson", "Robert Havlin",
};

static const std::array<std::string, 12> sample_trainer_names = {
	"Ali Stronge",    "Ian Williams", "William Knight",  "Milton Bradley",
	"Stuart Kittow",  "Mark Usher",   "Milton Harris",   "David Evans",
	"Seamus Mullins", "Gary Moore",   "Michael Appleby", "Adam West",
};
} // namespace janus_test
