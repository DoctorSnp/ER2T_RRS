#include <QDir>
#include "er2t.h"
#include "train-horn.h"
#include "filesystem.h"
#include "sl2m.h"
#include <QThread>

/*положение реверса*/
constexpr int REVERSE_BACKWARD = -1;
constexpr int REVERSE_FORWARD = 1;
constexpr int REVERSE_NEUTRAL = 0;

static qint32 MAIN_RESERVOIR_VOLUME = 1200;

static void printer (QString text)
{
    printf("%s\n", text.toStdString().c_str());
    fflush(stdout);
}


Er2T::Er2T(QObject *parent) : Vehicle (parent)
  , charge_press(0.0)
{
    initialization();
}

Er2T::~Er2T()
{
    printer("Delete Er2t");
}

void Er2T::initBrakeDevices(double p0, double pTM, double pFL)
{
    main_reservoir->setY(0, pFL);
    charge_press = p0;
    load_brakes_config(config_dir + QDir::separator() + "brakes-init.xml");
}


void Er2T::initialization()
{
    FileSystem &fs = FileSystem::getInstance();
    QString modules_dir = QString(fs.getModulesDir().c_str());
    //printer("Initializing Er2T");
    initSupplyMachines();

    m_initBrakeControls(modules_dir);
    m_initBrakeMechanics();
    m_initBrakeEquipment(modules_dir);

    initOtherEquipment();

    printer("Er2T is initialized!");
}

void Er2T::initOtherEquipment()
{
    if (horn == nullptr) {
        horn = new TrainHorn(this);
        connect(horn, &TrainHorn::soundSetVolume, this, &Er2T::soundSetVolume);
    }
}

void Er2T::initSupplyMachines()
{
    m_phase_spliter = new PhaseSplitter();
    connect(m_phase_spliter, &PhaseSplitter::soundSetPitch, this, &Er2T::soundSetPitch);

    for (size_t i = 0; i < m_motor_fans.size(); ++i)
    {
        m_motor_fans[i] = new MotorFan(i + 1);
        connect(m_motor_fans[i], &MotorFan::soundSetPitch, this, &Er2T::soundSetPitch);
    }

    main_reservoir = new Reservoir(static_cast<double>(MAIN_RESERVOIR_VOLUME) / 1000.0);

    QString mk_cfg_path = config_dir + QDir::separator() + "motor-compressor.xml";
    motor_compressor = new MotorCompressor(mk_cfg_path);
    connect(motor_compressor, &MotorCompressor::soundSetPitch, this, &Er2T::soundSetPitch);

    m_press_reg = new PressureRegulator();
}


void Er2T::load_brakes_config(QString path)
{
    CfgReader cfg;

    if (cfg.load(path))
    {
        QString secName = "BrakesState";

        double pFL = 0.0;

        if (cfg.getDouble(secName, "MainReservoirPressure", pFL))
        {
            main_reservoir->setY(0, pFL);
        }

        double k_flow = 0.0;

        if (cfg.getDouble(secName, "MainReservoirFlow", k_flow))
        {
            main_reservoir->setFlowCoeff(k_flow);
        }

        double ch_press = 0.0;

        if (cfg.getDouble(secName, "ChargingPressure", ch_press))
        {
            charge_press = ch_press;
        }

        int train_crane_pos = 6;

        if (cfg.getInt(secName, "TrainCranePos", train_crane_pos))
        {
            brake_crane->setPosition(train_crane_pos);
        }

        int loco_crane_pos = 0;

        if (cfg.getInt(secName, "LocoCranePos", loco_crane_pos))
        {
            loco_crane->setHandlePosition(loco_crane_pos);
        }

        int brake_lock = 0;

        int combine_crane_pos = -1;

        if (cfg.getInt(secName, "CombineCranePos", combine_crane_pos))
        {
            ubt->setCombineCranePos(combine_crane_pos);
        }

        if (cfg.getInt(secName, "BrakeLockDevice", brake_lock))
        {
            ubt->setState(brake_lock);

            if (brake_lock == 1)
            {
                ubt->setY(0, charge_press);
                brake_crane->init(charge_press, pFL);
                supply_reservoir->setY(0, charge_press);
            }
        }
    }
    else
        printer("Тормоза неинициализированы");
}

double Er2T::trac_char(double v)
{
    // Переводим номинальную скорость в м/с
         double v_nom = V_nom / Physics::kmh;
         double traction_force = 0;

         // Вычисляем силу тяги, в зависимости от величины
         // текущей скорости по отношению к номинальной.
         // Учитываем, что параметры заданной характеристики мы брали в кН,
         // а движку требуются величины в системе СИ, поэтому домножаем на 1000
         // переводя килоньютоны в ньютоны
         if (abs(v) < v_nom)
         {
             traction_force = (F_max + (V_nom - F_max) * abs(v) / v_nom) * 1000.0;
         }
         else
         {
             traction_force = V_nom * v_nom * 1000.0 / v;
         }

         return traction_force;
}

void Er2T::keyProcess()
{

    //if ( ( m_lastKeyTime + 1000 ) < m_timer.toMSecsSinceEpoch() )
    //    return;
    //printer(QString::number(m_timer.toMSecsSinceEpoch()));
   // m_lastKeyTime = m_timer.toMSecsSinceEpoch();

    if (getKeyState(KEY_A))
    {
        ref_traction_level += 0.01;
    }


    if (getKeyState(KEY_D))
    {
        ref_traction_level -= 0.01;
    }

    if (getKeyState(KEY_B))
    {
        //printer("Svistok!");
        emit soundSetVolume("Svistok", 100);
    }

    // Включение/выключение мотор-компрессора
    if (getKeyState(KEY_E))
    {
        if (isShift())
            m_mk_tumbler.set();
        else
            m_mk_tumbler.reset();
    }

    //m_checkKranPos();
    m_checkReverse();

    ref_traction_level = cut(ref_traction_level, 0.0, 1.0);
}

void Er2T::loadConfig(QString cfg_path)
{
    // Создаем экземпляр "читателя" XML-конфигов
    CfgReader cfg;

    // Открываем конфигурационный файл по переданному движком пути
    if (cfg.load(cfg_path))
    {
        // Задаем имя секции конфига, из которой будем читать параметры
        QString sectionName = "Vehicle";

        // Читаем интересующие нас параметы в соотвествующие переменные
        cfg.getDouble(sectionName, "F_max", F_max);
        cfg.getDouble(sectionName, "F_min", F_min);
        cfg.getDouble(sectionName, "V_nom", V_nom);
    }
}

void Er2T::m_initBrakeControls(QString modules_dir)
{
    ubt = new BrakeLock();
    ubt->read_config("ubt367m");
    connect(ubt, &BrakeLock::soundPlay, this, &Er2T::soundPlay);

    brake_crane = loadBrakeCrane(modules_dir + QDir::separator() + "krm395");
    if (brake_crane == nullptr)
    {
        printer("Cannot alloc brake_crane");
        return;
    }

    brake_crane->read_config("krm395");
    connect(brake_crane, &BrakeCrane::soundPlay, this, &Er2T::soundPlay);

    loco_crane = loadLocoCrane(modules_dir + QDir::separator() + "kvt254");
    loco_crane->read_config("kvt254");
}

void Er2T::m_initBrakeMechanics()
{
    m_trolley_mech[TROLLEY_FWD] = new TrolleyBrakeMech(config_dir +
                                      QDir::separator() +
                                      "motor-brakes-mech.xml");

    m_trolley_mech[TROLLEY_BWD] = new TrolleyBrakeMech(config_dir +
                                      QDir::separator() +
                                      "motor-brakes-mech.xml");
}

void Er2T::m_initBrakeEquipment(QString modules_dir)
{
    switch_valve = new SwitchingValve();
    switch_valve->read_config("zpk");

    pneumo_relay = new PneumoReley();
    pneumo_relay->read_config("rd304");

    pneumo_splitter = new PneumoSplitter();
    pneumo_splitter->read_config("pneumo-splitter");
    supply_reservoir = new Reservoir(0.078);

    //FileSystem &fs = FileSystem::getInstance();
    //brake_mech = loadBrakeMech(modules_dir + fs.separator() + "carbrakes-mech");
    //brake_mech->read_config("carbrakes-mech-composite");
    air_dist = loadAirDistributor(modules_dir +  QDir::separator() + "vr242");
    air_dist->read_config("vr242");
}

void Er2T::m_initTriggers()
{
    m_triggers.push_back(&fr_tumbler);
    m_triggers.push_back(&m_mk_tumbler);

    for (size_t i = 0; i < m_mv_tumblers.size(); ++i)
        m_triggers.push_back(&m_mv_tumblers[i]);

}

void Er2T::m_stepPhaseSplitter(double t, double dt)
{
    //double U_power = trac_trans->getU_sn() *
    //        static_cast<double>(fr_tumbler.getState());
    //m_phase_spliter->setU_power(U_power);
    m_phase_spliter->step(t, dt);
}

void Er2T::m_stepMotorCompressor(double t, double dt)
{
    main_reservoir->setAirFlow(motor_compressor->getAirFlow());
    main_reservoir->step(t, dt);

    motor_compressor->setExternalPressure(main_reservoir->getPressure());
    motor_compressor->setU_power(m_phase_spliter->getU_out() * static_cast<double>(m_mk_tumbler.getState()) * m_press_reg->getState());
    motor_compressor->step(t, dt);

    m_press_reg->setPressure(main_reservoir->getPressure());
    m_press_reg->step(t, dt);
}

void Er2T::m_stepBrakeControl(double t, double dt)
{
    // Подключаем к УБТ трубопровод от ГР
    ubt->setLocoFLpressure(main_reservoir->getPressure());
    // Подключаем к УБТ трубопровод ТМ от КрМ
    ubt->setCraneTMpressure(brake_crane->getBrakePipeInitPressure());
    ubt->setControl(keys);
    // Задаем давление в начале ТМ
    p0 = ubt->getLocoTMpressure();
    ubt->step(t, dt);

    brake_crane->setFeedLinePressure(ubt->getCraneFLpressure());
    brake_crane->setChargePressure(charge_press);
    brake_crane->setBrakePipePressure(pTM);
    brake_crane->setControl(keys);
    brake_crane->step(t, dt);

    loco_crane->setFeedlinePressure(ubt->getCraneFLpressure());
    loco_crane->setBrakeCylinderPressure(switch_valve->getPressure2());
    loco_crane->setAirDistributorFlow(0.0);
    loco_crane->setControl(keys);
    loco_crane->step(t, dt);
}

void Er2T::stepTrolleysBrakeMech(double t, double dt)
{
    switch_valve->setInputFlow1(air_dist->getBrakeCylinderAirFlow());
    switch_valve->setInputFlow2(loco_crane->getBrakeCylinderFlow());
    switch_valve->setOutputPressure(pneumo_splitter->getP_in());
    switch_valve->step(t, dt);

    // Тройник подключен к ЗПК
    pneumo_splitter->setQ_in(switch_valve->getOutputFlow());
    pneumo_splitter->setP_out1(pneumo_relay->getWorkPressure());
    pneumo_splitter->setP_out2(m_trolley_mech[TROLLEY_BWD]->getBrakeCylinderPressure());
    pneumo_splitter->step(t, dt);

    pneumo_relay->setPipelinePressure(main_reservoir->getPressure());
    pneumo_relay->setWorkAirFlow(pneumo_splitter->getQ_out1());
    pneumo_relay->setBrakeCylPressure(m_trolley_mech[TROLLEY_FWD]->getBrakeCylinderPressure());
    pneumo_relay->step(t, dt);

    // Передняя тележка наполняется через реле давления 304

    m_trolley_mech[TROLLEY_FWD]->setAirFlow(pneumo_relay->getBrakeCylAirFlow());
    m_trolley_mech[TROLLEY_FWD]->setVelocity(velocity);
    m_trolley_mech[TROLLEY_FWD]->step(t, dt);

    // Задняя тележка подключена через тройник от ЗПК
    m_trolley_mech[TROLLEY_BWD]->setAirFlow(pneumo_splitter->getQ_out2());
    m_trolley_mech[TROLLEY_BWD]->setVelocity(velocity);
    m_trolley_mech[TROLLEY_BWD]->step(t, dt);

    // передняя тележка
    Q_r[0] = m_trolley_mech[TROLLEY_FWD]->getBrakeTorque() * 100;
    Q_r[1] = m_trolley_mech[TROLLEY_FWD]->getBrakeTorque() * 100;
    Q_r[2] = m_trolley_mech[TROLLEY_FWD]->getBrakeTorque() * 100;

    // задняя тележка
    Q_r[3] = m_trolley_mech[TROLLEY_BWD]->getBrakeTorque() * 100;
    Q_r[4] = m_trolley_mech[TROLLEY_BWD]->getBrakeTorque() * 100;

    //QString str = "Brake1: "+ QString::number(Q_r[1]) + " Brake2: "+ QString::number(Q_r[2]);
    //printer(str);
}

void Er2T::stepAirDistributors(double t, double dt)
{
    supply_reservoir->setAirFlow(air_dist->getAirSupplyFlow());
    supply_reservoir->step(t, dt);

    air_dist->setBrakeCylinderPressure(switch_valve->getPressure1());
    air_dist->setAirSupplyPressure(supply_reservoir->getPressure());
    air_dist->setBrakepipePressure(pTM);
    auxRate = air_dist->getAuxRate();
    air_dist->step(t, dt);
}

void Er2T::m_registration(double t, double dt)
{
    if (next_vehicle == Q_NULLPTR)
        return;
    double dx = railway_coord  - next_vehicle->getRailwayCoord();
    QString line = QString("%1 %2")
            .arg(t)
            .arg(a[0]);
}

void Er2T::m_stepOtherEquipment(double t, double dt)
{
    horn->setControl(keys);
    horn->step(t, dt);
}

void Er2T::m_stepMotorFans(double t, double dt)
{
    for (size_t i = 0; i < NUM_MOTOR_FANS; ++i)
    {
        MotorFan *mf = m_motor_fans[i];
        mf->setU_power(m_phase_spliter->getU_out() *
                       static_cast<double>(m_mv_tumblers[i].getState()));

        mf->step(t, dt);
    }
}

void Er2T::m_checkKranPos()
{
    //printer("Kran: " + brake_crane->getPositionName() + " :" + QString::number(brake_crane->getHandlePosition() ));
}

void Er2T::step(double t, double dt)
{
    // Вычисляем силу тяги, которую реализует локомотив в данный момент
    double trac_force = ref_traction_level * trac_char(velocity);

    // Вычисляем момент, приходящийся на одну колесную пару
    double torque = trac_force * wheel_diameter / 2.0 / num_axis;
    for (size_t i = 1; i < Q_a.size(); ++i)
    {
        Q_a[i] = m_reversePos * (ref_traction_level * torque);
    }

    m_stepPhaseSplitter(t, dt);
    m_stepMotorFans(t, dt);
    m_stepMotorCompressor(t, dt);
    m_stepBrakeControl(t, dt);
    stepTrolleysBrakeMech(t, dt);
    stepAirDistributors(t, dt);
    m_stepOtherEquipment(t, dt);

    DebugMsg = QString("Гл.рез:%1 Зад. тяга:%2 Скор:%3 С.тяги:%4  "
                       "Давл:%5 ТЦ1:%6 ТЦ2:%7 Колодки:%8 pTM:%9 Коорд:%10")
            .arg(main_reservoir->getPressure(), 4, 'f', 2)
            .arg(ref_traction_level, 4, 'f', 2)
            .arg(velocity * Physics::kmh, 6, 'f', 2)
            .arg(trac_force / 1000.0, 6, 'f', 1)
            .arg(supply_reservoir->getPressure(), 4, 'f', 2)
            .arg(m_trolley_mech[TROLLEY_FWD]->getBrakeCylinderPressure(), 4, 'f', 2)
            .arg(m_trolley_mech[TROLLEY_BWD]->getBrakeCylinderPressure(), 4, 'f', 2)
            .arg(m_trolley_mech[TROLLEY_FWD]->getShoeForce() / 1000.0, 5, 'f', 1)
            .arg(pTM, 5, 'f', 1)
            .arg(getRailwayCoord(), 5, 'f', 1);
    ;
    DebugMsg += "|| " + QString::number(F_max);

    m_registration(t, dt);
}

void Er2T::m_checkReverse()
{
    if ( (velocity * Physics::kmh) > 0.0) // на ходу реверс не перетыкаем!
        return;
    if (getKeyState(KEY_R))
    {
        m_reversePos = REVERSE_BACKWARD;
    }

    if (getKeyState(KEY_F))
    {
        m_reversePos = REVERSE_FORWARD;
    }

    if (getKeyState(KEY_N))
    {
        m_reversePos = REVERSE_NEUTRAL;
    }
}

GET_VEHICLE(Er2T)
