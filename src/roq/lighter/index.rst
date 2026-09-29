.. _roq-lighter:

.. |dagger| unicode:: U+2020
.. |double-dagger| unicode:: U+2021
.. |right-arrow| unicode:: U+2192
.. |right-double-arrow| unicode:: U+21D2
.. |left-right-double-arrow| unicode:: U+21D4
.. |check-mark| unicode:: U+2705
.. |cross-mark| unicode:: U+274C
.. |negative-cross-mark| unicode:: U+274E
.. |footnote-1| unicode:: U+2776
.. |footnote-2| unicode:: U+2777
.. |footnote-3| unicode:: U+2778


`roq-lighter <https://github.com/roq-trading/roq-lighter/>`__
==============================================================

.. tab:: Unstable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/unstable \
           roq-lighter

.. tab:: Stable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/stable \
           roq-lighter


Supports
--------

.. grid::  2
  :gutter: 2

  .. grid-item-card::  Products

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:enumerator:`Spot <roq::SecurityType::SPOT>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Futures <roq::SecurityType::FUTURES>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Swap <roq::SecurityType::SWAP>`
        - |check-mark|
        -
      * - :cpp:enumerator:`Option <roq::SecurityType::OPTION>`
        - |check-mark|
        -

  .. grid-item-card::  Market Data

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`ReferenceData <roq::ReferenceData>`
        - |check-mark|
        -
      * - :cpp:class:`MarketStatus <roq::MarketStatus>`
        - |check-mark|
        -
      * - :cpp:class:`TopOfBook <roq::TopOfBook>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByPrice <roq::MarketByPriceUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`MarketByOrder <roq::MarketByOrderUpdate>`
        - |cross-mark|
        -
      * - :cpp:class:`TradeSummary <roq::TradeSummary>`
        - |check-mark|
        -
      * - :cpp:class:`Statistics <roq::StatisticsUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`TimeSeries <roq::TimeSeriesUpdate>`
        - |check-mark|
        -

  .. grid-item-card::  Orders & Quotes

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`CreateOrder <roq::CreateOrder>`
        - |check-mark|
        -
      * - :cpp:class:`ModifyOrder <roq::ModifyOrder>`
        - |check-mark|
        -
      * - :cpp:class:`CancelOrder <roq::CancelOrder>`
        - |check-mark|
        -
      * - :cpp:class:`CancelAllOrders <roq::CancelAllOrders>`
        - |check-mark|
        -
      * - :cpp:class:`MassQuote <roq::MassQuote>`
        - |cross-mark|
        -
      * - :cpp:class:`CancelQuotes <roq::CancelQuotes>`
        - |cross-mark|
        -

  .. grid-item-card::  Account

    .. list-table::
      :widths: auto
      :align: left

      * - :cpp:class:`Funds <roq::FundsUpdate>`
        - |check-mark|
        -
      * - :cpp:class:`Position <roq::PositionUpdate>`
        - |check-mark|
        -


.. note::

   |check-mark| = Available.

   |negative-cross-mark| = Not implemented.

   |cross-mark| = Unavailable.

   |footnote-1| = The exchange protocol does not support streaming updates for reference data and market status.


Using
-----

.. code-block:: shell

   $ roq-lighter [FLAGS]


.. _roq-lighter-flags:

Flags
-----

.. code-block:: shell

   $ roq-lighter --help

.. tab:: Flags

   .. include:: flags/flags.rstinc

.. tab:: REST

   .. include:: flags/rest.rstinc

.. tab:: WS

   .. include:: flags/ws.rstinc

.. tab:: Download

   .. include:: flags/download.rstinc

.. tab:: MBP

   .. include:: flags/mbp.rstinc

.. tab:: Request

   .. include:: flags/request.rstinc

.. tab:: Misc

   .. include:: flags/misc.rstinc


Environments
------------

.. tab:: Prod

   .. code-block:: shell

      $ --flagfile $CONDA_PREFIX/share/roq-lighter/flags/prod/flags.cfg

   .. include:: flags/prod/flags.cfg
     :code: shell


Configuration
-------------

.. code-block:: shell

   $ --config_file $CONDA_PREFIX/share/roq-lighter/config.toml

.. important::

   This template will be replaced when the software is upgraded.
   Make a copy and modify to your own needs.

.. include:: config.toml
   :code: toml


Market Data
-----------


Inbound
~~~~~~~

.. tab:: TradingStatus

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - Enum
       -
       -

     * - :code:`PreLaunch`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`Trading`
       - |right-double-arrow|
       - :cpp:enumerator:`OPEN <roq::TradingStatus::OPEN>`

     * - :code:`Settling`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`Delivering`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::TradingStatus::UNDEFINED>`

     * - :code:`Closed`
       - |right-double-arrow|
       - :cpp:enumerator:`CLOSE <roq::TradingStatus::CLOSE>`


.. tab:: StatisticsType

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - Event
       - Field
       -
       -

     * - :code:`Tickers`
       - :code:`highPrice24h`
       - |right-double-arrow|
       - :cpp:enumerator:`HIGHEST_TRADED_PRICE <roq::StatisticsType::HIGHEST_TRADED_PRICE>`

     * - :code:`Tickers`
       - :code:`lowPrice24h`
       - |right-double-arrow|
       - :cpp:enumerator:`LOWEST_TRADED_PRICE <roq::StatisticsType::LOWEST_TRADED_PRICE>`

     * - :code:`Tickers`
       - :code:`lastPrice`
       - |right-double-arrow|
       - :cpp:enumerator:`CLOSE_PRICE <roq::StatisticsType::CLOSE_PRICE>`

     * - :code:`Tickers`
       - :code:`volume24h`
       - |right-double-arrow|
       - :cpp:enumerator:`TRADE_VOLUME <roq::StatisticsType::TRADE_VOLUME>`


Order Management
----------------


Inbound
~~~~~~~

.. tab:: OrderType

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - :code:`orderType`
       -
       -

     * - :code:`Market`
       - |right-double-arrow|
       - :cpp:enumerator:`MARKET <roq::OrderType::MARKET>`

     * - :code:`Limit`
       - |right-double-arrow|
       - :cpp:enumerator:`LIMIT <roq::OrderType::LIMIT>`


.. tab:: TimeInForce

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - :code:`timeInForce`
       -
       -

     * - :code:`GTC`
       - |right-double-arrow|
       - :cpp:enumerator:`GTC <roq::TimeInForce::GTC>`

     * - :code:`FOK`
       - |right-double-arrow|
       - :cpp:enumerator:`FOK <roq::TimeInForce::FOK>`

     * - :code:`IOC`
       - |right-double-arrow|
       - :cpp:enumerator:`IOC <roq::TimeInForce::IOC>`


.. tab:: OrderStatus

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - :code:`orderStatus`
       -
       -

     * - :code:`Created`
       - |right-double-arrow|
       - :cpp:enumerator:`WORKING <roq::OrderStatus::WORKING>`

     * - :code:`New`
       - |right-double-arrow|
       - :cpp:enumerator:`WORKING <roq::OrderStatus::WORKING>`

     * - :code:`Rejected`
       - |right-double-arrow|
       - :cpp:enumerator:`REJECTED <roq::OrderStatus::REJECTED>`

     * - :code:`PartiallyFilled`
       - |right-double-arrow|
       - :cpp:enumerator:`WORKING <roq::OrderStatus::WORKING>`

     * - :code:`PartiallyFilledCanceled`
       - |right-double-arrow|
       - :cpp:enumerator:`CANCELED <roq::OrderStatus::CANCELED>`

     * - :code:`Filled`
       - |right-double-arrow|
       - :cpp:enumerator:`COMPLETED <roq::OrderStatus::COMPLETED>`

     * - :code:`Cancelled`
       - |right-double-arrow|
       - :cpp:enumerator:`CANCELED <roq::OrderStatus::CANCELED>`

     * - :code:`Untriggered`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::OrderStatus::UNDEFINED>`

     * - :code:`Triggered`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::OrderStatus::UNDEFINED>`

     * - :code:`Deactivated`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::OrderStatus::UNDEFINED>`

     * - :code:`Active`
       - |right-double-arrow|
       - :cpp:enumerator:`UNDEFINED <roq::OrderStatus::UNDEFINED>`


Outbound
~~~~~~~~

.. tab:: CreateOrder

   .. list-table::
     :header-rows: 1
     :widths: auto
     :align: left

     * - :cpp:member:`order_type <roq::CreateOrder::order_type>`
       - :cpp:member:`execution_instructions <roq::CreateOrder::execution_instructions>`
       - :cpp:member:`price <roq::CreateOrder::price>`
       - :cpp:member:`stop_price <roq::CreateOrder::stop_price>`
       -
       - :code:`orderType`
       - :code:`price`
       - :code:`reduceOnly`

     * - :cpp:enumerator:`MARKET <roq::OrderType::MARKET>`
       -
       - :code:`NaN`
       - :code:`NaN`
       - |right-double-arrow|
       - :code:`Market`
       -
       - :code:`false`

     * - :cpp:enumerator:`MARKET <roq::OrderType::MARKET>`
       - :cpp:enumerator:`DO_NOT_INCREASE <roq::ExecutionInstruction::DO_NOT_INCREASE>`
       - :code:`NaN`
       - :code:`NaN`
       - |right-double-arrow|
       - :code:`Market`
       - |cross-mark|
       - :code:`true`

     * - :cpp:enumerator:`LIMIT <roq::OrderType::LIMIT>`
       -
       - |check-mark|
       - :code:`NaN`
       - |right-double-arrow|
       - :code:`Limit`
       - |check-mark|
       - :code:`false`

     * - :cpp:enumerator:`LIMIT <roq::OrderType::LIMIT>`
       - :cpp:enumerator:`DO_NOT_INCREASE <roq::ExecutionInstruction::DO_NOT_INCREASE>`
       - |check-mark|
       - :code:`NaN`
       - |right-double-arrow|
       - :code:`Limit`
       - |check-mark|
       - :code:`true`


.. tab:: ModifyOrder

   TBD


.. tab:: CancelOrder

   TBD


.. tab:: CancelAllOrders

   TBD


Comments
--------

* API market data subscription requires re-subscription of all symbols.
  This means that new symbol discovery is a very heavy operation.


References
----------

Common
~~~~~~

* :ref:`Using Conda <tutorial-conda>`
* :ref:`Using Flags <abseil-cpp>`
* :ref:`Gateway Flags <gateway-flags>`
* :ref:`Gateway Config <gateway-config>`


GitHub
~~~~~~

* `roq-lighter <https://github.com/roq-trading/roq-lighter/>`__


Exchange
~~~~~~~~

* `Website <https://lighter.com/>`__
* `Documentation <https://docs.lighter.com/>`__
