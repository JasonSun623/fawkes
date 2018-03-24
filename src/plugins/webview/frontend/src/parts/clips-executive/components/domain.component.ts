
// Copyright  2018  Tim Niemueller <niemueller@kbsg.rwth-aachen.de>
// License: Apache 2.0 (http://www.apache.org/licenses/LICENSE-2.0)

import { Component, OnInit } from '@angular/core';
import { Router, ActivatedRoute, ParamMap } from '@angular/router';
import { Observable } from 'rxjs/Observable';
import { catchError, tap, switchMap } from 'rxjs/operators';
import 'rxjs/add/observable/forkJoin';
import 'rxjs/add/observable/interval';
import 'rxjs/add/operator/catch';
import { HttpClient, HttpHeaders } from '@angular/common/http';

import { ConfigurationService } from '../../../services/config.service';
import { ClipsExecutiveApiService } from '../services/api.service';
import { Goal } from '../models/Goal';
import { Plan } from '../models/Plan';
import { PlanAction } from '../models/PlanAction';
import { DomainOperator } from '../models/DomainOperator';
import { DomainPrecondition } from '../models/DomainPrecondition';
import { DomainPreconditionAtom } from '../models/DomainPreconditionAtom';
import { DomainPreconditionCompound } from '../models/DomainPreconditionCompound';


@Component({
  selector: 'clips-executive-domain',
  templateUrl: './domain.component.html',
  styleUrls: ['./domain.component.scss']
})
export class DomainComponent implements OnInit {

  constructor(private config: ConfigurationService,
              private http: HttpClient,
              private api_service : ClipsExecutiveApiService) { }

  loading = false;

  domain_data = null;
  auto_refresh_subscription = null;

  zero_message_facts      = "No facts received";
  zero_message_predicates = "No predicates received";
  zero_message_objects    = "No objects received";
  zero_message_operators  = "No operators received";
  
  displayed_fact_columns   = [ "fact_name", "fact_params" ];
  displayed_object_columns = [ "object_name", "object_type" ];
  displayed_op_columns = [ "op_name", "op_params", "op_wait_sensed" ];
  displayed_pred_columns = [ "pred_name", "pred_params", "pred_sensed" ];
  
  ngOnInit() {
    this.reset_domain_data();
    this.refresh_domain_data();
  }

  reset_domain_data()
  {
    this.domain_data = {
      operators:      {},
      operators_arr:  [],
      predicates:     {},
      predicates_arr: [],
      objects:        [],
      facts:          []
    };
  }
  
  refresh_domain_data()
  {
    this.loading = true;
    Observable
      .forkJoin([
        // forkjoin here to allow for requesting multiple items
        this.api_service.list_domain_operators(),
        this.api_service.list_domain_predicates(),
        this.api_service.list_domain_objects(),
        this.api_service.list_domain_facts(),
      ])
      .map((data: any[]) => {
        return { operators:      data[0].reduce((map,obj) => (map[obj.name]=obj, map), {}),
                 operators_arr:  data[0],
                 predicates:     data[1].reduce((map,obj) => (map[obj.name]=obj, map), {}),
                 predicates_arr: data[1],
                 objects:        data[2],
                 facts:          data[3].sort((a,b) => (a.name < b.name))};
      })
      .subscribe(
        (dd) => {
          console.log(`Received domain data: ${Object.keys(dd.operators).length} operators, `+
                      `${Object.keys(dd.predicates).length} predicates, ${dd.objects.length} objects, `+
                      `${dd.facts.length} facts`);
          this.domain_data = dd;
          this.loading = false;
          if (dd.facts.length == 0) {
            this.zero_message_facts = "No facts in current domain";
          }
          if (dd.predicates_arr.length == 0) {
            this.zero_message_predicates = "No predicates in current domain";
          }
          if (dd.operators_arr.length == 0) {
            this.zero_message_operators = "No operators in current domain";
          }
          if (dd.objects.length == 0) {
            this.zero_message_objects = "No objects in current domain";
          }
        },
        (err) => {
          this.loading = false;
          console.log("Failed to receive domain data");
          this.zero_message_facts = this.zero_message_predicates =
            this.zero_message_operators = this.zero_message_objects =
            "API server unavailable. Robot down?";
        }
      );
  }
  
  private enable_autorefresh()
  {
    if (this.auto_refresh_subscription)  return;
    this.auto_refresh_subscription =
      Observable.interval(2000).subscribe((num) => {
        this.refresh_domain_data();
      });
    this.refresh_domain_data();
  }

  private disable_autorefresh()
  {
    if (this.auto_refresh_subscription) {
      this.auto_refresh_subscription.unsubscribe();
      this.auto_refresh_subscription = null;
    }
  }

  toggle_autorefresh()
  {
    if (this.auto_refresh_subscription) {
      this.disable_autorefresh();
    } else {
      this.enable_autorefresh();
    }
  }

}
