#ifndef TEST_LOCO_H
#define TEST_LOCO_H

#include <QDateTime>

#include "vehicle-api.h"
#include "ubt367m.h"
#include "reservoir.h"

/* пневматика */
#include "pneumo-splitter.h"
#include "pneumatic/trolley-brake-mech.h"
#include "pneumatic/motor-fan.h"
#include "pneumatic/motor-compressor.h"
#include "pneumatic/pressure-regulator.h"

#include "network.h"
/*электрика*/

#include "electric/phase-splitter.h"

#include    "brake-mech.h" // Модель тормозной рычажной передачи
#include    "reservoir.h" // Модель резервуара
#include    "airdistributor.h" // Базовый класс "воздухораспределитель"

class Er2T : public Vehicle
{
 public:
     Er2T(QObject *parent = Q_NULLPTR);
     ~Er2T() override;
     /// Инициализация тормозных приборов
     void initBrakeDevices(double p0, double pTM, double pFL) override;
 private:

     /* инициализация*/
    void initialization() override;
    void initSupplyMachines();
    void load_brakes_config(QString path);
    void initOtherEquipment();
    // ======== устройства
    /// Переключательный клапан ЗПК
    SwitchingValve  *switch_valve;

    /// Реле давления усл. №304
    PneumoReley     *pneumo_relay;

    /// Разветвитель трубопроводов (тройник)
    PneumoSplitter  *pneumo_splitter;
   // SL2M    *m_speedMeter = nullptr;
    TrainHorn   *horn = nullptr;

    // Тормозная рычажная передача
    //BrakeMech   *brake_mech;

    // Запасный резервуар
    Reservoir   *supply_reservoir;

    // Воздухан
    AirDistributor *air_dist;

    enum
    {
        NUM_MOTOR_FANS = 2,
        MV1 = 0,
        MV2 = 1,
    };

    /// Регулятор давления в ГР
    PressureRegulator *m_press_reg;

    /// Асинхронный расщепитель фаз
    PhaseSplitter   *m_phase_spliter;

    /// Мотор-вентиляторы
    std::array<MotorFan *, NUM_MOTOR_FANS> m_motor_fans;

    /// Мотор-компрессор
    MotorCompressor *motor_compressor;

    /// Главный резервуар
    Reservoir   *main_reservoir;
    /// Зарядное давление
    double  charge_press;
    /// Устройство блокировки тормозов усл. №367М
    BrakeLock   *ubt;
    /// Поездной кран машиниста (КрМ)
    BrakeCrane  *brake_crane;
    /// Кран впомогательного тормоза (КВТ)
    LocoCrane   *loco_crane;

    double trac_char(double v);
    /// Шаг моделирования систем локомотива
    void step(double t, double dt) override;
    // Обработка нажатия клавиш
    void keyProcess() override;
    // Загрузка пользовательских параметров из конфига
    void loadConfig(QString cfg_path) override;

    /// Инициализация приборов управления тормозами
    void m_initBrakeControls(QString modules_dir);

    /// Инициализация тормозной рычажной передачи
    void m_initBrakeMechanics();

    void m_initBrakeEquipment(QString modules_dir);

    void m_initTriggers();

    void m_stepPhaseSplitter(double t, double dt);

    void m_stepMotorCompressor(double t, double dt);

    void m_stepBrakeControl(double t, double dt);

    void m_stepMotorFans(double t, double dt);

    void stepTrolleysBrakeMech(double t, double dt);

    void stepAirDistributors(double t, double dt);

    // пошаговое выполнение
    void m_stepOtherEquipment(double t, double dt);
    // Максимальная реализуемая сила тяги, кН
    double F_max = 450.0;
     /// Номинальная сила тяги (соотвествующая продолжительной мощности), кН
    double F_min = 350.0;
     /// Номинальная скорость (соотвествующая часовой мощности), км/ч
    double V_nom = 80.0;
    /// Заданный уровень тягового усилия
    double ref_traction_level = 0.0;


    // мои функции - потом перенести
    void m_checkKranPos();
    void m_checkReverse();
    enum
    {
        NUM_TROLLEYS = 2,
        TROLLEY_FWD = 0,
        TROLLEY_BWD = 1
    };

    enum
    {
        NUM_MOTORS = 4,
        TED1 = 0,
        TED2 = 1,
        TED3 = 2,
        TED4 = 3
    };

    /// Тяговый трансформатор
    //TracTransformer *trac_trans;

    std::vector<Trigger *> m_triggers;

    /// Тормозные механизмы тележек
    std::array<TrolleyBrakeMech *, NUM_TROLLEYS> m_trolley_mech;

    /// Триггеры тумблеров управления мотор-вентиляторами
    std::array<Trigger, NUM_MOTOR_FANS> m_mv_tumblers;


    /// Тригер тумблера "Фазорасщепитель"
    Trigger fr_tumbler;
    /// Тригер тумблера управления мотор-компрессором
    Trigger m_mk_tumbler;
    int m_reversePos = 0;

    int m_kranPos = 0;
    qint64 m_lastKeyTime = 0;
    //QDateTime m_timer;
 };

#endif // TEST_LOCO_CPP
