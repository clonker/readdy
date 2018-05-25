/********************************************************************
 * Copyright © 2018 Computational Molecular Biology Group,          *
 *                  Freie Universität Berlin (GER)                  *
 *                                                                  *
 * This file is part of ReaDDy.                                     *
 *                                                                  *
 * ReaDDy is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU Lesser General Public License as   *
 * published by the Free Software Foundation, either version 3 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU Lesser General Public License for more details.              *
 *                                                                  *
 * You should have received a copy of the GNU Lesser General        *
 * Public License along with this program. If not, see              *
 * <http://www.gnu.org/licenses/>.                                  *
 ********************************************************************/


/**
 * @file integration.h
 * @brief Routines for numerically integrating functions
 * @author chrisfroe
 * @date 24.05.18
 * @copyright GNU Lesser General Public License v3.0
 */

#pragma once

#include <limits>
#include <readdy/common/common.h>
#include <readdy/common/numeric.h>
#include <queue>

namespace readdy {
namespace util {
namespace integration {

constexpr std::array<scalar, 101> weightsGaussKronrod201 = {
        0.00012796430957024721771296604719777847195024888314,
        0.00035867672428027546451819698699002695000334366552,
        0.00061229953852751686968468056548070834614262353518,
        0.00085841483623935311664167374912297679663629462243,
        0.00109796416132059890255397465155433695320668680377,
        0.00133983780660382341754863019304498885110952126241,
        0.00158539114055099209687612715239439499710635284278,
        0.00182937001345660807993702363601705354484262856545,
        0.00207023170435975873786445531287183441533108495909,
        0.00231123008487149478622574422551013428738043877307,
        0.00255360720035108161107860930247813105889546630505,
        0.00279496754149865554132924610296587377583807221606,
        0.00303416468796449379917327454133990046860593313429,
        0.00327287902049999951685865323103972155152040933937,
        0.00351196946843139471347132584863839017075918804593,
        0.00375002082356796252973315848571211234815922368507,
        0.00398619744931235184550297813723407560532468657842,
        0.00422153240339578242265345682700445192052466529462,
        0.00445663655824494510224150713533978978713271961637,
        0.00469055184548748439481506164708745018600910075929,
        0.00492264171642177778021256522028596859149482883314,
        0.00515360688570317554082069990644640034295769630886,
        0.00538389880957824664615043517317289909958011793608,
        0.00561281343110871597561315793834039501636153255547,
        0.00583984643867452187979812998241983823823136309861,
        0.00606550320973303863921033440183559298376742116472,
        0.00629012776973767504951202123451843743076930245062,
        0.00651317411315535135623979587712921532296014614460,
        0.00673423042383435906546110501890771393375794286618,
        0.00695367647707533778618333444634079607272882365501,
        0.00717178053315253341524750684836420331388733863189,
        0.00738810306985931173687401288774170241141466081296,
        0.00760230005117430829817610049263739864681050864527,
        0.00781466520662034505085711719016354399072460775306,
        0.00802541132258648471308962366547616916200657491446,
        0.00823417501731623690079536275494453055496288986340,
        0.00844066393687280440093304148919420862913744658522,
        0.00864511004867877912656139959329107529390995475094,
        0.00884768427998831020007201725385439489287388704377,
        0.00904808029993671228835071165243690477400271230057,
        0.00924604652391111724687570119177700799209325218608,
        0.00944176898875185552314903500699258800726938238310,
        0.00963538626179462391106485411435148851979409901308,
        0.00982663642373088065571326334283803573470396510601,
        0.01001530101161305053268537450183723383477657205865,
        0.01020153095088495986015919422291444604170764482781,
        0.01038543929706851271926198711586114579021961800284,
        0.01056679983247565098373215943168121363502057278619,
        0.01074542170778693080646692567379385062368419611030,
        0.01092142841500597232724852424011180927323636614697,
        0.01109491257094606883971232155924910988765741030054,
        0.01126567746332956237923986454364445975020263538030,
        0.01143355579967040516909625306002457417492109489982,
        0.01159864925643444521982733110763019057800050447511,
        0.01176103385541580107532667409174481684699820604850,
        0.01192053786750440393445381605750602287206317500254,
        0.01207701451438569994891221026442585729555689105328,
        0.01223054787113266210109559371404225931637605012182,
        0.01238120033058132478094426900828573870330496911285,
        0.01252882177740552594442502987847792106610336206449,
        0.01267328363219971069040254068582595376734875571756,
        0.01281465559402401820734387481377863160543256944343,
        0.01295298874891947508181321347643329979562489424432,
        0.01308815204304083947131249628348670851036112224233,
        0.01322003331459425763799456688719462334189620391816,
        0.01334869039477997219532795119897057181197377482865,
        0.01347416490042760653129384849738331885547322480541,
        0.01359634288062617797837336787649308030170304524353,
        0.01371512721021576932690771893822620264523809273465,
        0.01383056582631592119521912896609004389004651368400,
        0.01394269234050930765052410813271065280007760112975,
        0.01405140838748433908894290183169630886722307263276,
        0.01415663080386553141797847993528261640630526208010,
        0.01425839919822706151742984801432832105691149679315,
        0.01435674034589556885653893978408636354274396518129,
        0.01445157028450311174994356610608770124062851800510,
        0.01454281897629966555537908242478331367093974940807,
        0.01463051894733967823881185958389904589719486669315,
        0.01471469106719224107616070689745858786072765279902,
        0.01479526485272091523273713296861772987622838214603,
        0.01487218274551550851114896175149252660492892913239,
        0.01494547117901571779314143269144247770437604078025,
        0.01501514584992601775929409587145443755032002323588,
        0.01508114903502136704084960300677971411706204873991,
        0.01514343516330660012211995270464269800292539698574,
        0.01520202535514477439125881646888569025211919463334,
        0.01525693069928626681152511683204470629971399732269,
        0.01530810567790395821753937559915116616801682403691,
        0.01535551634411844871530936744265760333339456402763,
        0.01539917910754267008730746329865897219178834017417,
        0.01543910086714820098073247575245749712612887092387,
        0.01547524789208464777691069202608380676296793168412,
        0.01550759760659890369589537474066318545807510752952,
        0.01553616215856523293822391298200717744826237954682,
        0.01556094454341676462938645995315419106586887459681,
        0.01558192251438053678351080364024221646030414904878,
        0.01559908471168710670698089639871094604147763682664,
        0.01561243933403481989468460959619688421126306083370,
        0.01562198563724487346851127872839698584387203112330,
        0.01562771265700169003321136142695117550866696845783,
        0.01562962018460484993210603370275515394376090869371
};

constexpr std::array<scalar, 101> abscissaeGaussKronrod201 = {
        0.99995250325234874194558759586872675230201032186296,
        0.99971372677344123367822846934230067671834952730840,
        0.99922816588380125603468689462902570985161537549449,
        0.99849195063959581840016335918634916230485485042057,
        0.99751361312272973925231594907328863008404380861059,
        0.99629513473312514918613173224113103543643128814043,
        0.99483262193692678214229984365392976193623306275107,
        0.99312493703744345965200989284878347073177145886652,
        0.99117498765102584468434111430887815181054026590303,
        0.98898439524299174800441874580773663183933363710695,
        0.98655201560314858543214329463373901430393735434646,
        0.98387754070605701549610015551100816734436701685080,
        0.98096284258061262094794383303858458336270892150820,
        0.97780935848691828855378108842920192863523449426625,
        0.97441692579132830191156952676419699735650628441883,
        0.97078577576370633193089785789750538855055719947821,
        0.96691753657125109640782216846696070235549924264129,
        0.96281365425581552729365932603016638643733150673041,
        0.95847452546444300989234207911968140986589441225372,
        0.95390078292549174284933693089435764464522145101467,
        0.94909405007764308807002661063238399920305087971147,
        0.94405587013625597796277470641521874673972037338210,
        0.93878704353080861159581415143454540129149440268213,
        0.93328853504307954592433366813086250408354607429702,
        0.92756205890488017513109041315467906526813611874056,
        0.92160929814533395266695132848198745912458279773220,
        0.91543138327874017569217880264508697295963148874566,
        0.90902957098252969046712633778914606444327728958459,
        0.90240571020837873902681113376935748186170440444374,
        0.89556164497072698669852102243022776984818176899772,
        0.88849879400991357098633030987871529168085698938796,
        0.88121867938501841557331682542780558244549441102126,
        0.87372330591888454232901721359869700713999812944025,
        0.86601468849716462341073996967624296638031483930559,
        0.85809450656528158256769359279995027785553637603413,
        0.84996452787959128429336259142010465407379077950067,
        0.84162692263159422803428490828685310746728377281023,
        0.83308387988840082354291583384475567990749483030996,
        0.82433731934785511555382066394577351308957189674952,
        0.81538923833917625439398875864925800538255037651370,
        0.80624197522998149898858119947772518581699983645897,
        0.79689789239031447638957288218324598288952685596429,
        0.78735913298215327121915096807989317961453460725592,
        0.77762790964949547562755138683449010653853979993608,
        0.76770672752390789522106835290910988619423163187483,
        0.75759811851970717603566796443840077231310897190004,
        0.74730443440010216722543283418354970398268442664090,
        0.73682808980202070551242771482010100284327844624712,
        0.72617175233016540458785543702154726618162507726741,
        0.71533811757305644645996712270436596408439783859563,
        0.70432973201100171130158275065992755531836865624424,
        0.69314919935580196594864794167543726558700001793073,
        0.68179934329913790750199866457132454892446683289894,
        0.67028301560314101580258701432322661366980568402882,
        0.65860294402143308554822559819231646499591256882672,
        0.64676190851412927983263030445863043501973378424853,
        0.63476288086691287961700686682300223062843045115192,
        0.62260886020370777160419084517231224465381773228982,
        0.61030274223394973052304014680666051705840336769498,
        0.59784747024717872126480654514934063639489919232049,
        0.58524615484722223751330407348246631581348705331813,
        0.57250193262138119131687044352572544896003394967556,
        0.55961785385987287004483883274572350988535188007656,
        0.54659701206509416746799425718174990395624177593753,
        0.53344264632810924391925934260941164781644110851200,
        0.52015801988176305664681574945520853076893769042009,
        0.50674632405992146013910672747973470564589036548717,
        0.49321078920819093356930879344933399099072332535856,
        0.47955477168853305490480433055889014820419073581020,
        0.46578164977335804224921662339575458161165111021221,
        0.45189474208663614330483287246151294430151576448075,
        0.43789740217203151310897804362219596212570176348410,
        0.42379309169388210181506692715279269905002159373794,
        0.40958529167830154252886840005715770149536438916475,
        0.39527743404816671610367673557861642687518028284140,
        0.38087298162462995676336254886958740374970726512371,
        0.36637548876362800231863519558827019975726941799314,
        0.35178852637242172097234382954897056524931809638907,
        0.33711562544519365010380511464074285765053795714571,
        0.32236034390052915172247658239832542740219162302309,
        0.30752631562753657120752788303988100046876429120433,
        0.29261718803847196473755588823549438456153898917258,
        0.27763657675183495616346814290881136285663496860995,
        0.26258812037150347916892933625498214113202269453552,
        0.24747551869798892096614882491742687591540876456389,
        0.23230248184497396964950996320796411069750977150714,
        0.21707269541726899854706722200296707797219389992710,
        0.20178986409573599723604885953039646294369200355905,
        0.18645773958437819065168393518669992652130113816299,
        0.17108008053860327488753237470708980746585972511807,
        0.15566062775725995230028233532247535151338751890639,
        0.14020313723611397320751460468240551661687300626336,
        0.12471139826072812045287156735452075155899823333651,
        0.10918920358006111500342600657938488688489962996916,
        0.09364033428354398098988456682945235155973991630929,
        0.07806858281343663669481737120155257397635002744853,
        0.06247776146923010010357548046587278110387007259524,
        0.04687168242159163161492391293384830953706539908602,
        0.03125415208386678081857435045254965009647374420346,
        0.01562898442154308287221669999742934014775618285556,
        0.00000000000000000000000000000000000000000000000000
};

constexpr std::array<scalar, 50> abscissaeGauss201 = {
        0.99971372677344123367822846934230067671834952730840,
        0.99849195063959581840016335918634916230485485042057,
        0.99629513473312514918613173224113103543643128814043,
        0.99312493703744345965200989284878347073177145886652,
        0.98898439524299174800441874580773663183933363710695,
        0.98387754070605701549610015551100816734436701685080,
        0.97780935848691828855378108842920192863523449426625,
        0.97078577576370633193089785789750538855055719947821,
        0.96281365425581552729365932603016638643733150673041,
        0.95390078292549174284933693089435764464522145101467,
        0.94405587013625597796277470641521874673972037338210,
        0.93328853504307954592433366813086250408354607429702,
        0.92160929814533395266695132848198745912458279773220,
        0.90902957098252969046712633778914606444327728958459,
        0.89556164497072698669852102243022776984818176899772,
        0.88121867938501841557331682542780558244549441102126,
        0.86601468849716462341073996967624296638031483930559,
        0.84996452787959128429336259142010465407379077950067,
        0.83308387988840082354291583384475567990749483030996,
        0.81538923833917625439398875864925800538255037651370,
        0.79689789239031447638957288218324598288952685596429,
        0.77762790964949547562755138683449010653853979993608,
        0.75759811851970717603566796443840077231310897190004,
        0.73682808980202070551242771482010100284327844624712,
        0.71533811757305644645996712270436596408439783859563,
        0.69314919935580196594864794167543726558700001793073,
        0.67028301560314101580258701432322661366980568402882,
        0.64676190851412927983263030445863043501973378424853,
        0.62260886020370777160419084517231224465381773228982,
        0.59784747024717872126480654514934063639489919232049,
        0.57250193262138119131687044352572544896003394967556,
        0.54659701206509416746799425718174990395624177593753,
        0.52015801988176305664681574945520853076893769042009,
        0.49321078920819093356930879344933399099072332535856,
        0.46578164977335804224921662339575458161165111021221,
        0.43789740217203151310897804362219596212570176348410,
        0.40958529167830154252886840005715770149536438916475,
        0.38087298162462995676336254886958740374970726512371,
        0.35178852637242172097234382954897056524931809638907,
        0.32236034390052915172247658239832542740219162302309,
        0.29261718803847196473755588823549438456153898917258,
        0.26258812037150347916892933625498214113202269453552,
        0.23230248184497396964950996320796411069750977150714,
        0.20178986409573599723604885953039646294369200355905,
        0.17108008053860327488753237470708980746585972511807,
        0.14020313723611397320751460468240551661687300626336,
        0.10918920358006111500342600657938488688489962996916,
        0.07806858281343663669481737120155257397635002744853,
        0.04687168242159163161492391293384830953706539908602,
        0.01562898442154308287221669999742934014775618285556
};

constexpr std::array<scalar, 50> weightsGauss201 = {
        0.00073463449050567173040632065833033639067047356248,
        0.00170939265351810523952935837149119524373138549146,
        0.00268392537155348241943959042900112008193111495100,
        0.00365596120132637518234245872752519569920656740515,
        0.00462445006342211935109578908297847665035249529489,
        0.00558842800386551515721194634843921073131869400808,
        0.00654694845084532276415210333149526369993836336648,
        0.00749907325546471157882874401639778316358347894815,
        0.00844387146966897140262083490230100193464445988410,
        0.00938041965369445795141823766081211873078704323867,
        0.01030780257486896958578210172783537797605834384143,
        0.01122511402318597711722157336633358477722641956438,
        0.01213145766297949740774479244874817073696312331126,
        0.01302594789297154228555858375890179013496473584175,
        0.01390771070371877268795414910800463779518081214312,
        0.01477588452744130176887998752035425716938874311460,
        0.01562962107754600272393686595379192555246997980994,
        0.01646808617614521264310498008821078082116766160380,
        0.01729046056832358243934419836674167481162350856517,
        0.01809594072212811666439075142049303134757874495839,
        0.01888373961337490455294116588154323429711127634742,
        0.01965308749443530586538147024544406555526959949125,
        0.02040323264620943276683885165758377060570969930262,
        0.02113344211252764154267230044096968163532972887451,
        0.02184300241624738631395374130439802476534899982325,
        0.02253122025633627270179697093167396234015893534871,
        0.02319742318525412162248885418272728845115448573609,
        0.02384096026596820596256041190228343214470744909262,
        0.02446120270795705271997502334977289064629573239780,
        0.02505754448157958970376422562092326422383855852793,
        0.02562940291020811607564200986215087092697767002027,
        0.02617621923954567634230874175730188501127513119069,
        0.02669745918357096266038466418633635063465575039001,
        0.02719261344657688013649156780217069226698789601200,
        0.02766119822079238829420415587042645529240035866422,
        0.02810275565910117331764833018699455045141809940021,
        0.02851685432239509799093676286445787325984272548397,
        0.02890308960112520313487622813451526531560786805526,
        0.02926108411063827662011902349564095444308419504535,
        0.02959048805991264251175451067883658517280628507137,
        0.02989097959333283091683680666859582765809141426080,
        0.03016226510516914491906868161047923265710232578271,
        0.03040407952645482001650785981882517660560724831012,
        0.03061618658398044849645944326205319285308602378906,
        0.03079837903115259042771390303055976009497083447037,
        0.03095047885049098823406346347074792738298717776694,
        0.03107233742756651658781017024291803484591543634796,
        0.03116383569620990678381832121718665334383636868393,
        0.03122488425484935773237649864809813488180274068218,
        0.03125542345386335694764247438619802878783383672609
};

/**
 * Perform integration of f(x) for x in [lowerLimit, upperLimit] with a Gauss quadrature using
 * tabulated abscissae and weights from a Gauss-Kronrod rule with 201 nodes. Also estimate an error
 * as the difference of the obtained integral and a value integrated with fewer nodes. For the latter use the
 * the associated Gauss rule, whose abscissae are a subset of the 201 Gauss-Kronrod nodes.
 *
 * @tparam Func Callable with one scalar argument returning a scalar
 * @param f the integrand
 * @param lowerLimit lower boundary of integration
 * @param upperLimit upper boundary of integration
 * @return a pair of (value of the integral, error estimate of the integral)
 */
template<typename Func>
inline std::pair<scalar, scalar> integrate(Func f, scalar lowerLimit, scalar upperLimit) {
    // create another function h(x) = c*f(g(x)), to rescale boundaries from [a,b] to [-1,1]
    // h obeys I_{-1}^{1}(h) = I_{a}^{b}(f),
    // i.e. the integral of h over [-1,1] is equal to the integral of f over [a,b].
    // g(x)=0.5*(b-a)*x + 0.5(a+b) takes care of rescaling of integration limits of the arguments
    // and c=0.5*(b-a) is a multiplicative constant
    const scalar midpoint = (lowerLimit + upperLimit) / 2.;
    const scalar halfInterval = (upperLimit - lowerLimit) / 2.;

    const auto integrand = [&f, &midpoint, &halfInterval](const scalar x) {
        const scalar rescaledArgument = halfInterval * x + midpoint;
        return halfInterval * f(rescaledArgument);
    };

    scalar integralKronrod = weightsGaussKronrod201.back() * integrand(0.);
    scalar integralGauss = 0.;
    // leave out last kronrod index, since it corresponds to abscissa=0
    for (std::size_t kronrodIndex = 0; kronrodIndex < abscissaeGaussKronrod201.size() - 1; ++kronrodIndex) {
        const scalar abscissa = abscissaeGaussKronrod201[kronrodIndex];
        const scalar kronrodWeight = weightsGaussKronrod201[kronrodIndex];
        const scalar symmetricIntegrandSum = integrand(abscissa) + integrand(-1. * abscissa);

        integralKronrod += symmetricIntegrandSum * kronrodWeight;

        if ((kronrodIndex + 1) % 2 == 0) {
            // every second abscissa of the GaussKronrod rule is also a Gauss abscissa
            const std::size_t gaussIndex = (kronrodIndex - 1) / 2;
            const scalar gaussWeight = weightsGauss201[gaussIndex];

            integralGauss += symmetricIntegrandSum * gaussWeight;
        }
    }

    const scalar absoluteErrorEstimate = std::abs(integralKronrod - integralGauss);
    return std::make_pair(integralKronrod, absoluteErrorEstimate);
};

struct Panel {
    explicit Panel(scalar lowerLimit, scalar upperLimit, scalar integral, scalar absoluteErrorEstimate)
            : lowerLimit(lowerLimit), upperLimit(upperLimit), integral(integral),
              absoluteErrorEstimate(absoluteErrorEstimate) {};

    bool operator<(const Panel &other) const {
        return absoluteErrorEstimate < other.absoluteErrorEstimate;
    };

    scalar lowerLimit;
    scalar upperLimit;
    scalar integral;
    scalar absoluteErrorEstimate;
};

template<typename Func>
inline std::pair<scalar, scalar> integrateAdaptive(Func f, scalar lowerLimit, scalar upperLimit,
                                scalar desiredError = std::numeric_limits<scalar>::epsilon(),
                                std::size_t maxiter = 100) {
    std::vector<Panel> panels;

    const auto initialResult = integrate(f, lowerLimit, upperLimit);
    panels.push_back(Panel(lowerLimit, upperLimit, initialResult.first, initialResult.second));

    scalar totalError = panels.front().absoluteErrorEstimate;
    scalar totalIntegral = panels.front().integral;

    std::size_t iter = 0;
    while (totalError > desiredError && iter < maxiter) {
        std::sort(panels.begin(), panels.end());

        {
            const auto &panel = panels.front();
            const scalar midpoint = (panel.lowerLimit + panel.upperLimit) / 2.;

            const auto lowerResult = integrate(f, panel.lowerLimit, midpoint);
            Panel lowerPanel(panel.lowerLimit, midpoint, lowerResult.first, lowerResult.second);

            const auto upperResult = integrate(f, midpoint, panel.upperLimit);
            Panel upperPanel(midpoint, panel.upperLimit, upperResult.first, upperResult.second);

            panels.erase(panels.begin());
            panels.push_back(lowerPanel);
            panels.push_back(upperPanel);
        }

        {
            totalError = std::accumulate(panels.begin(), panels.end(), 0.,
                                         [](scalar sum, const Panel &p) { return sum + p.absoluteErrorEstimate; });
            totalIntegral = std::accumulate(panels.begin(), panels.end(), 0.,
                                         [](scalar sum, const Panel &p) { return sum + p.integral; });
        }

        ++iter;
    }

    return std::make_pair(totalIntegral, totalError);
}

}
}
}