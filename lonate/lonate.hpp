/**
 *  @file lonate.hpp
 *  @copyright defined in eos/LICENSE.txt
 *  @author    Geujnoo LEE, gjlee@lonate.io
 *  @date       2018-11-06
 *  @version  0.0.1
 */

#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <cmath>
#include "../lonate.token/lonate.token.hpp"



class[[eosio::contract]] lonate : public eosio::contract
{

  public:
    lonate(eosio::name receiver, eosio::name code, eosio::datastream<const char *> ds) : eosio::contract(receiver, code, ds), offers(receiver, code.value), comments(receiver, code.value),  accounts(receiver, code.value), fulfillments(receiver, code.value)
    {}

    [[eosio::action]] void version();
    [[eosio::action]] void offer( eosio::name author, const eosio::asset &quantity, const std::string subject, const std::string context, const std::string category, const std::string pictures, const uint32_t endtime);
    [[eosio::action]] void coffer();
    [[eosio::action]] void comment(eosio::name author, const uint64_t offerid, const std::string context);
    [[eosio::action]] void ccomment();
    [[eosio::action]] void deposit(eosio::name from, const eosio::asset &quantity);
    [[eosio::action]] void donate(eosio::name from, const uint64_t offerid, const eosio::asset &quantity);
    [[eosio::action]] void curate(eosio::name from, const uint64_t offer_id);
    [[eosio::action]] void requestexec(eosio::name, const uint64_t offer_id, const eosio::asset &quantity, const std::string purpose);
    [[eosio::action]] void expire(eosio::name author, const uint64_t offerid);
    [[eosio::action]] void deleterecord(const eosio::name from, const uint64_t offerid);
    [[eosio::action]] void fulfill(eosio::name requestor, const uint64_t offerid, const eosio::asset &quantity, const std::string context);             
private:
    const eosio::symbol_code LWON = eosio::symbol("LWON", 4).code();
    const eosio::symbol_code LOTY = eosio::symbol("LOTY", 4).code();

    struct [[eosio::table]] fulfillment
    {
        uint64_t fulfillid;
        eosio::name requestor;
        uint64_t offerid;
         eosio::asset quantity;
         std::string context;
        uint32_t created = now();
        uint64_t primary_key() const { return fulfillid; }
        uint64_t by_account() const { return requestor.value; }

    };

    typedef eosio::multi_index<"fulfillment"_n, fulfillment,
                               eosio::indexed_by<"requestor"_n, eosio::const_mem_fun<fulfillment, uint64_t, &fulfillment::by_account>>>
        fulfillment_index;

    struct [[eosio::table]] profferg
    {
    
        uint64_t offerid;
        eosio::asset offer_balance = eosio::asset(0, eosio::symbol("LWON",4));
        eosio::asset target_balance;
        eosio::name author;
        std::string context;
        std::string subject;
        std::string category;
        std::string pictures;
        float curated = 0.00f;
        float offer_power = 100.00f;
        uint8_t success = 0;
        uint32_t created = now();
        uint32_t endtime;
        uint32_t state=0;
        uint64_t primary_key() const { return offerid; }
    };
    
    typedef eosio::multi_index<"profferg"_n, profferg> offer_index;

    struct [[eosio::table]] procomment
    {
        uint64_t commentid;
        uint64_t offerid;
        eosio::name author;
        std::string context;
        uint64_t primary_key() const { return commentid; }
        uint64_t by_offerid() const { return offerid; }
    };

    typedef eosio::multi_index<"procomment"_n, procomment,
                               eosio::indexed_by<"offerid"_n, eosio::const_mem_fun<procomment, uint64_t, &procomment::by_offerid>>>
        comment_index;


    struct [[eosio::table]] accountd
    {
        eosio::name owner;
        eosio::asset LWON_balance = eosio::asset(0, eosio::symbol("LWON",4));
        eosio::asset LOTY_balance = eosio::asset(0,eosio::symbol("LOTY",4));
        uint32_t comment_cnt = 0;
        uint32_t post_cnt = 0;
        uint32_t curate_cnt = 0;
        uint64_t social_power = 0;
        uint64_t tot_donation = 0;
        float vote_power = 100.00f;
        uint32_t last_vote = now();
        int8_t grade = 0;
        bool is_empty() const { return !(LWON_balance.amount); }
        uint64_t primary_key() const { return owner.value; }
    };

    typedef eosio::multi_index<"accountd"_n, accountd> account_index;

    /// local instances of the multi indexes
    offer_index offers;
    comment_index comments;
    account_index accounts;
    fulfillment_index fulfillments;

    void transfer(const eosio::name from, const eosio::asset &quantity);
    void camcontrol(const eosio::name author, const uint64_t offerid, const uint32_t end);
    eosio::asset getbalance(eosio::name author){
        return eosio::token::get_balance("lonate.token"_n, author, LWON);
    };
    eosio::asset getsupply(){
        return eosio::token::get_supply("lonate.token"_n, LWON);
    }

    float getcurpower(eosio::name from){
        return 10/(1+(exp(-((float)10.5*(float)getbalance(from).amount/(float)getsupply().amount-(float)2.2))));
    }
    //void checkvp(std::const_iterator *acnt_itr);
    //void checkop(auto &acnt_itr);
};